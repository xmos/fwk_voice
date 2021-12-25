
#include "xs3_math.h"
#include "stage_a_wrapper.h"
#define AEC_MAX_Y_CHANNELS (2)
#define AEC_MAX_X_CHANNELS (2)
#define AEC_FRAME_ADVANCE (240)

#include "ap_stage_a_state.h"

extern void ap_stage_a_init(ap_stage_a_state *state);
extern void ap_stage_a(ap_stage_a_state *state,
    int32_t (*input_y_data)[AP_FRAME_ADVANCE],
    int32_t (*input_x_data)[AP_FRAME_ADVANCE],
    int32_t (*output_data)[AP_FRAME_ADVANCE]);

extern int32_t debug_set_delay;
int32_t *debug_signed_delay_ptr_1 = &debug_set_delay;

void stage_a_wrapper(const char *input_file_name, const char* output_file_name) {
    //For logging delay to file..
    static FILE *out_file;
    char * delay_file_name = "xc_sim_delays.txt";
    out_file = fopen((char *)delay_file_name, "wt");
    if (out_file == NULL)
    {
        printf("Error while opening output file: %s", delay_file_name);
        _Exit(1);
    }

    //aec_state_t aec_state;
    //adec_state_t adec_state;    

    file_t input_file, output_file;
    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open output wav file that will contain the AEC output
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct, output_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in att_get_wav_header_details()\n");
        _Exit(1);
    }

    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 32bit wav file
    if(input_header_struct.bit_depth != 32)
     {
         printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header_struct.bit_depth, input_file_name);
         _Exit(1);
     }
    // Ensure input wav file contains correct number of channels 
    if(input_header_struct.num_channels != (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)){
        printf("Error: wav num channels(%d) does not match aec(%u)\n", input_header_struct.num_channels, (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS));
        _Exit(1);
    }
    
    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / AEC_FRAME_ADVANCE;
    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            AEC_MAX_Y_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count*AEC_FRAME_ADVANCE);

    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);

    int32_t input_read_buffer[AEC_FRAME_ADVANCE * (AEC_MAX_Y_CHANNELS + AEC_MAX_X_CHANNELS)] = {0}; // Array for storing interleaved input read from wav file
    int32_t output_write_buffer[AEC_FRAME_ADVANCE * (AEC_MAX_Y_CHANNELS)];

    int32_t DWORD_ALIGNED frame_y[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED frame_x[AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED stage_a_output[2][AP_FRAME_ADVANCE];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);
    
    //Initialise ap_stage_a
    ap_stage_a_state DWORD_ALIGNED stage_a_state;
    ap_stage_a_init(&stage_a_state);

    for(unsigned b=0;b<block_count;b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * AEC_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* AEC_FRAME_ADVANCE);
        // Deinterleave and copy y and x samples to their respective buffers
        for(unsigned f=0; f<AEC_FRAME_ADVANCE; f++){
            for(unsigned ch=0;ch<AEC_MAX_Y_CHANNELS;ch++){
                unsigned i =(f * (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)) + ch;
                frame_y[ch][f] = input_read_buffer[i];
            }
            for(unsigned ch=0;ch<AEC_MAX_X_CHANNELS;ch++){
                unsigned i =(f * (AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS)) + AEC_MAX_Y_CHANNELS + ch;
                frame_x[ch][f] = input_read_buffer[i];
            }
        }
        ap_stage_a(&stage_a_state, frame_y, frame_x, stage_a_output);
        
        // Create interleaved output that can be written to wav file
        for (unsigned ch=0;ch<AEC_MAX_Y_CHANNELS;ch++){
            for(unsigned i=0;i<AEC_FRAME_ADVANCE;i++){
                output_write_buffer[i*(AEC_MAX_Y_CHANNELS) + ch] = stage_a_output[ch][i];
            }
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * AEC_FRAME_ADVANCE * AEC_MAX_Y_CHANNELS);
        
        //This will get called once per frame so rate is correct
          int32_t signed_delay = *debug_signed_delay_ptr_1;
#if TEST_WAV_XSCOPE
          //fprintf is not supported over xscope. printf to stdout instead and then parse
          //print with xc_sim_delays: tag to differtiate from other prints coming out of the device
          printf("xc_sim_delays: %d\n", signed_delay);
          fflush(stdout);
#else
          fprintf(out_file, "%d\n", signed_delay);
          fflush(out_file);
#endif
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}
