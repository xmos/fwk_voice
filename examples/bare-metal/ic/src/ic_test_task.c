// Copyright 2022 XMOS LIMITED.
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

#include "ic_api.h"

#include "xmath/xmath.h"
#include "fileio.h"
#include "wav_utils.h"
#include "dump_var_py.h"

//Optionally quit processing after number of frames processed
// #define MAX_FRAMES  0

#define OUTPUT_CHANNELS 2 //0 is IC output and 1 is beamformed output

#if PROFILE_PROCESSING
#include "profile.h"
#endif

void ic_task(const char *input_file_name, const char *output_file_name) {
    //open files
    file_t input_file, output_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");\
    // file_t dut_var_file;
    // ret = file_open(&dut_var_file, "dut_var.py", "wb"); //Option to dump variables on each frame
    // assert((!ret) && "Failed to open file");

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

    if(input_header_struct.num_channels != (IC_Y_CHANNELS+IC_X_CHANNELS)){
        printf("Error: wav num channels(%d) does not match ic(%u)\n", input_header_struct.num_channels, (IC_Y_CHANNELS+IC_X_CHANNELS));
        _Exit(1);
    }
    

    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    unsigned block_count = frame_count / IC_FRAME_ADVANCE;

#if MAX_FRAMES
    if(block_count > MAX_FRAMES){
        block_count = MAX_FRAMES;
    }
#endif

    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            OUTPUT_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count*IC_FRAME_ADVANCE);

    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);

    int32_t input_read_buffer[IC_FRAME_ADVANCE * (IC_Y_CHANNELS + IC_X_CHANNELS)] = {0};
    int32_t output_write_buffer[IC_FRAME_ADVANCE * OUTPUT_CHANNELS] = {0};

    int32_t DWORD_ALIGNED frame_y[IC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED frame_x[IC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED output[IC_FRAME_ADVANCE];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    //Start ic
    prof(0, "start_ic_init");
    ic_state_t state;
    ic_init(&state);
    prof(1, "end_ic_init"); 

    #if DISABLE_ADAPTION_CONTROLLER
    state.ic_adaption_controller_state.adaption_controller_config.adaption_config = IC_ADAPTION_FORCE_ON;
    state.leakage_alpha = f32_to_float_s32(1.0); //From test_wav_ic
    #endif

    
    for(unsigned b=0;b<block_count;b++){
        //printf("frame %d of %d\n", b, block_count);
        long input_location =  wav_get_frame_start(&input_header_struct, b * IC_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* IC_FRAME_ADVANCE);
        for(unsigned f=0; f<IC_FRAME_ADVANCE; f++){
            for(unsigned ch=0;ch<IC_Y_CHANNELS;ch++){
                unsigned i = (f * (IC_Y_CHANNELS+IC_X_CHANNELS)) + ch;
                frame_y[f] = input_read_buffer[i];
            }
            for(unsigned ch=0;ch<IC_X_CHANNELS;ch++){
                unsigned i =(f * (IC_Y_CHANNELS+IC_X_CHANNELS)) + IC_Y_CHANNELS + ch;
                frame_x[f] = input_read_buffer[i];
            }
        }

        prof(2, "start_ic_filter");
        // Call IC functions to process IC_FRAME_ADVANCE new samples of data
        ic_filter(&state,  frame_y, frame_x, output);
        prof(3, "end_ic_filter");

        prof(4, "start_vad_estimate");
        float_s32_t vnr = {0,0};
        prof(5, "end_vad_estimate");

        prof(6, "start_ic_adapt");
        ic_adapt(&state, vnr);
        prof(7, "end_ic_adapt");

        for(unsigned i=0;i<IC_FRAME_ADVANCE;i++){
            output_write_buffer[i*OUTPUT_CHANNELS] = output[i]; //IC adaptive filter output
            int32_t y_samp = input_read_buffer[i * (IC_Y_CHANNELS+IC_X_CHANNELS) + 0];
            int32_t x_samp = input_read_buffer[i * (IC_Y_CHANNELS+IC_X_CHANNELS) + 1];
            output_write_buffer[i*OUTPUT_CHANNELS + 1] = (y_samp >> 1) + (x_samp >> 1); //Beamform with no delay on 2nd channel
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * IC_FRAME_ADVANCE * OUTPUT_CHANNELS);
        print_prof(0,8,b+1);
    }
    file_close(&input_file);
    file_close(&output_file);
    // file_close(&dut_var_file);
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
    ic_task(IN_WAV_FILE_NAME, OUT_WAV_FILE_NAME);
}
#else //Linux build
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    ic_task(argv[1], argv[2]);
    return 0;
}
#endif