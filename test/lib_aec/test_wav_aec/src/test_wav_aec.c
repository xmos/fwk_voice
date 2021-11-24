// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if !X86_BUILD
#ifdef __XC__
    #define chanend_t chanend
#else
    #include <xcore/chanend.h>
#endif
#include <platform.h>
#include <print.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "aec_config.h"
#include "aec_schedule.h"
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_testapp.h"
#include "fileio.h"
#include "wav_utils.h"
#include "dump_H_hat.h"

#if PROFILE_PROCESSING
#include "profile.h"
#endif

#define ARG_NOT_SPECIFIED (-1)
typedef enum {
    Y_CHANNELS,
    X_CHANNELS,
    MAIN_FILTER_PHASES,
    SHADOW_FILTER_PHASES,
    ADAPTION_MODE,
    FORCE_ADAPTION_MU,
    STOP_ADAPTING,
    NUM_RUNTIME_ARGS
}runtime_args_indexes_t;

int runtime_args[NUM_RUNTIME_ARGS];

//valid_tokens_str entries and runtime_args_indexes_t need to maintain the same order so that when a runtime argument token string matches index 'i' string in valid_tokens_str, the corresponding
//value can be updated in runtime_args[i]
const char *valid_tokens_str[] = {"y_channels", "x_channels", "main_filter_phases", "shadow_filter_phases", "adaption_mode", "force_adaption_mu", "stop_adapting"}; //TODO autogenerate from runtime_args_indexes_t


#define MAX_ARGS_BUF_SIZE (1024)
void parse_runtime_args(int *runtime_args_arr) {
    file_t args_file;
    int ret = file_open(&args_file, "args.bin", "rb");
    if(ret != 0) {
        return;
    }
    char readbuf[MAX_ARGS_BUF_SIZE];

    int args_file_size = get_file_size(&args_file);
    printf("args_file_size = %d\n",args_file_size);
    if(!args_file_size) {
        file_close(&args_file);
        return;
    }
    file_read(&args_file, (uint8_t*)readbuf, args_file_size);
    readbuf[args_file_size] = '\0';

    //printf("args %s\n",readbuf);
    char *c = strtok(readbuf, "\n");
    while(c != NULL) {
        char token_str[100];
        int token_val;
        sscanf(c, "%s %d", token_str, &token_val);
        //printf("token %s\n",c);
        for(int i=0; i<sizeof(valid_tokens_str)/sizeof(valid_tokens_str[0]); i++) {
            if(strcmp(valid_tokens_str[i], token_str) == 0) {
                //printf("found token %s, value %d\n", valid_tokens_str[i], token_val);
                runtime_args_arr[i] = token_val;
            }
        }
        //printf("str %s val %d\n",token_str, token_val);
        c = strtok(NULL, "\n");
    }
}

#define Q1_30(f) ((int32_t)((double)(INT_MAX>>1) * f)) //TODO use lib_xs3_math use_exponent instead
void aec_task(const char *input_file_name, const char *output_file_name) {
    //check validity of compile time configuration
    assert(AEC_MAX_Y_CHANNELS <= AEC_LIB_MAX_Y_CHANNELS);
    assert(AEC_MAX_X_CHANNELS <= AEC_LIB_MAX_X_CHANNELS);
    assert((AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_MAIN_FILTER_PHASES) <= (AEC_LIB_MAX_Y_CHANNELS * AEC_LIB_MAX_X_CHANNELS * AEC_LIB_MAIN_FILTER_PHASES));
    assert((AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_SHADOW_FILTER_PHASES) <= (AEC_LIB_MAX_Y_CHANNELS * AEC_LIB_MAX_X_CHANNELS * AEC_LIB_SHADOW_FILTER_PHASES));
    //Initialise default values of runtime arguments
    runtime_args[Y_CHANNELS] = AEC_MAX_Y_CHANNELS;
    runtime_args[X_CHANNELS] = AEC_MAX_X_CHANNELS;
    runtime_args[MAIN_FILTER_PHASES] = AEC_MAIN_FILTER_PHASES;
    runtime_args[SHADOW_FILTER_PHASES] = AEC_SHADOW_FILTER_PHASES;
    runtime_args[ADAPTION_MODE] = AEC_ADAPTION_AUTO; //TODO Hardcoded!
    runtime_args[FORCE_ADAPTION_MU] = Q1_30(1.0); //TODO Hardcoded
    runtime_args[STOP_ADAPTING] = -1;
    parse_runtime_args(runtime_args);
    printf("runtime args = ");
    for(int i=0; i<NUM_RUNTIME_ARGS; i++) {
        printf("%d ",runtime_args[i]);
    }
    printf("\n");

    //Check validity of runtime configuration
    assert(runtime_args[Y_CHANNELS] <= AEC_MAX_Y_CHANNELS);
    assert(runtime_args[X_CHANNELS] <= AEC_MAX_X_CHANNELS);
    assert((runtime_args[Y_CHANNELS] * runtime_args[X_CHANNELS] * runtime_args[MAIN_FILTER_PHASES]) <= (AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_MAIN_FILTER_PHASES));
    assert((runtime_args[Y_CHANNELS] * runtime_args[X_CHANNELS] * runtime_args[SHADOW_FILTER_PHASES]) <= (AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_SHADOW_FILTER_PHASES));
    
    //open files
    file_t input_file, output_file, H_hat_file, delay_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&H_hat_file, "H_hat.bin", "wb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&delay_file, "delay.bin", "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct, output_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in att_get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    if(input_header_struct.bit_depth != 32)
     {
         printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header_struct.bit_depth, input_file_name);
         _Exit(1);
     }

    if(input_header_struct.num_channels != (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)){
        printf("Error: wav num channels(%d) does not match aec(%u)\n", input_header_struct.num_channels, (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS));
        _Exit(1);
    }
    

    unsigned frame_count = wav_get_num_frames(&input_header_struct);

    unsigned block_count = frame_count / AEC_FRAME_ADVANCE;
    //printf("num frames = %d\n",block_count);
    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            AEC_MAX_Y_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count*AEC_FRAME_ADVANCE);

    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);

    int32_t input_read_buffer[AEC_PROC_FRAME_LENGTH*(AEC_MAX_Y_CHANNELS + AEC_MAX_X_CHANNELS)] = {0};
    int32_t output_write_buffer[AEC_FRAME_ADVANCE * (AEC_MAX_Y_CHANNELS)];

    int32_t DWORD_ALIGNED frame_y[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];
    int32_t DWORD_ALIGNED frame_x[AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    //Start AEC
    prof(0, "start_aec_init");
    uint8_t DWORD_ALIGNED aec_memory_pool[sizeof(aec_memory_pool_t)];
    uint8_t DWORD_ALIGNED aec_shadow_filt_memory_pool[sizeof(aec_shadow_filt_memory_pool_t)]; 
    aec_state_t DWORD_ALIGNED main_state;
    aec_state_t DWORD_ALIGNED shadow_state;
    aec_shared_state_t DWORD_ALIGNED aec_shared_state;
    
    aec_init(&main_state, &shadow_state, &aec_shared_state,
            &aec_memory_pool[0], &aec_shadow_filt_memory_pool[0],
            runtime_args[Y_CHANNELS], runtime_args[X_CHANNELS],
            runtime_args[MAIN_FILTER_PHASES], runtime_args[SHADOW_FILTER_PHASES]);
    prof(1, "end_aec_init"); 

    main_state.shared_state->config_params.coh_mu_conf.adaption_config = runtime_args[ADAPTION_MODE];
    main_state.shared_state->config_params.coh_mu_conf.force_adaption_mu_q30 = runtime_args[FORCE_ADAPTION_MU];

    for(unsigned b=0;b<block_count;b++){
        //printf("frame %d\n",b);
        long input_location =  wav_get_frame_start(&input_header_struct, b * AEC_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[(AEC_PROC_FRAME_LENGTH-AEC_FRAME_ADVANCE)*(AEC_MAX_Y_CHANNELS+AEC_MAX_Y_CHANNELS)], bytes_per_frame* AEC_FRAME_ADVANCE);
        memset(frame_y, 0, sizeof(frame_y));
        memset(frame_x, 0, sizeof(frame_x));
        for(unsigned f=0; f<AEC_PROC_FRAME_LENGTH; f++){
            for(unsigned ch=0;ch<runtime_args[Y_CHANNELS];ch++){
                unsigned i =(f * (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)) + ch;
                frame_y[ch][f] = input_read_buffer[i];
            }
            for(unsigned ch=0;ch<runtime_args[X_CHANNELS];ch++){
                unsigned i =(f * (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)) + AEC_MAX_Y_CHANNELS + ch;
                frame_x[ch][f] = input_read_buffer[i];
            }
        }
        if (runtime_args[STOP_ADAPTING] > 0) {
            runtime_args[STOP_ADAPTING]--;
            if (runtime_args[STOP_ADAPTING] == 0) {
                aec_dump_H_hat(&main_state, &H_hat_file);
                //turn off adaption
                main_state.shared_state->config_params.coh_mu_conf.adaption_config = AEC_ADAPTION_FORCE_OFF;
            }
        }
        prof(2, "start_aec_process_frame");
        aec_testapp_process_frame(&main_state, &shadow_state, frame_y, frame_x);
        prof(3, "end_aec_process_frame");

        prof(4, "start_aec_estimate_delay");
        int delay = aec_estimate_delay(&main_state);
        prof(5, "end_aec_estimate_delay");

        char strbuf[100];
        sprintf(strbuf, "%d\n", delay);
        file_write(&delay_file, (uint8_t*)strbuf,  strlen(strbuf));

        for (unsigned ch=0;ch<runtime_args[Y_CHANNELS];ch++){
            for(unsigned i=0;i<AEC_FRAME_ADVANCE;i++){
                output_write_buffer[i*(AEC_MAX_Y_CHANNELS) + ch] = main_state.output[ch].data[i];
            }
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * AEC_FRAME_ADVANCE * AEC_MAX_Y_CHANNELS);

        for(unsigned i=0;i<(AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*(AEC_MAX_Y_CHANNELS + AEC_MAX_X_CHANNELS);i++){
            input_read_buffer[i] = input_read_buffer[i + AEC_FRAME_ADVANCE*(AEC_MAX_Y_CHANNELS + AEC_MAX_X_CHANNELS)];
        }
        print_prof(0,6,b+1);
    }
    file_close(&input_file);
    file_close(&output_file);
    file_close(&H_hat_file);
    file_close(&delay_file);
    shutdown_session();
}


#if !X86_BUILD
void main_tile1(chanend_t c_cross_tile)
{
    //Do nothing
}

#define IN_WAV_FILE_NAME    "input.wav"
#define OUT_WAV_FILE_NAME   "output.wav"
void main_tile0(chanend_t c_cross_tile, chanend_t xscope_chan)
{
#if TEST_WAV_XSCOPE
    xscope_io_init(xscope_chan);
#endif 
    aec_task(IN_WAV_FILE_NAME, OUT_WAV_FILE_NAME);
}
#else //Linux build
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    aec_task(argv[1], argv[2]);
    return 0;
}
#endif
