#include "xmath/xmath.h"
#include "fileio.h"
#include "wav_utils.h"

#include "pipeline_config.h"
#include "pipeline_state.h"
#if !X86_BUILD
    #include <xcore/channel.h>
    #include <xcore/chanend.h>
    #include <xcore/channel_transaction.h>
#else
    typedef int chanend_t; // To compile for x86
#endif

extern void pipeline_tile0_init(pipeline_state_tile0_t *state);
extern void pipeline_tile1_init(pipeline_state_tile1_t *state);

extern void pipeline_process_frame_tile0(pipeline_state_tile0_t *state,
        int32_t (*input_y_data)[AP_FRAME_ADVANCE],
        int32_t (*input_x_data)[AP_FRAME_ADVANCE],
        int32_t (*output_data)[AP_FRAME_ADVANCE],
        pipeline_metadata_t *md_output);

extern void pipeline_process_frame_tile1(pipeline_state_tile1_t *state, pipeline_metadata_t *md_input,
        int32_t (*input_data)[AP_FRAME_ADVANCE],
        int32_t (*output_data)[AP_FRAME_ADVANCE]);

void pipeline_wrapper_tile0(chanend_t c_pcm_out, chanend_t c_pcm_in, const char *input_file_name, const char* output_file_name)
{
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
        printf("error in get_wav_header_details()\n");
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
    if(input_header_struct.num_channels != (AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS)){
        printf("Error: wav num channels(%d) does not match aec(%u)\n", input_header_struct.num_channels, (AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS));
        _Exit(1);
    }
    
    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / AP_FRAME_ADVANCE;
    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            AP_MAX_Y_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count*AP_FRAME_ADVANCE);

    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);

    int32_t input_read_buffer[AP_FRAME_ADVANCE * (AP_MAX_Y_CHANNELS + AP_MAX_X_CHANNELS)] = {0}; // Array for storing interleaved input read from wav file
    int32_t output_write_buffer[AP_FRAME_ADVANCE * (AP_MAX_Y_CHANNELS)];

    int32_t DWORD_ALIGNED frame_y[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED frame_x[AP_MAX_X_CHANNELS][AP_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED pipeline_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);
    
    // Initialise pipeline
    pipeline_state_tile0_t DWORD_ALIGNED pipeline_tile0_state;
    pipeline_tile0_init(&pipeline_tile0_state);
    
    // Everything runs as part of one function on x86
#if X86_BUILD
    pipeline_state_tile1_t DWORD_ALIGNED pipeline_tile1_state;
    pipeline_tile1_init(&pipeline_tile1_state);
#endif

    for(unsigned b=0;b<block_count;b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * AP_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* AP_FRAME_ADVANCE);
        // Deinterleave and copy y and x samples to their respective buffers
        for(unsigned f=0; f<AP_FRAME_ADVANCE; f++){
            for(unsigned ch=0;ch<AP_MAX_Y_CHANNELS;ch++){
                unsigned i =(f * (AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS)) + ch;
                frame_y[ch][f] = input_read_buffer[i];
            }
            for(unsigned ch=0;ch<AP_MAX_X_CHANNELS;ch++){
                unsigned i =(f * (AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS)) + AP_MAX_Y_CHANNELS + ch;
                frame_x[ch][f] = input_read_buffer[i];
            }
        }
        
        pipeline_metadata_t md;
        int32_t DWORD_ALIGNED tile0_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
        pipeline_process_frame_tile0(&pipeline_tile0_state, frame_y, frame_x, tile0_output, &md);

#if X86_BUILD
        // For x86, just call everything from one function
        pipeline_process_frame_tile1(&pipeline_tile1_state, &md, tile0_output, pipeline_output);
#else
        // Send data to process to the other tile and receive processed output back
        //Transfer to other tile
        chan_out_buf_byte(c_pcm_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_out_buf_word(c_pcm_out, (uint32_t*)&tile0_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
        //Receive back from the other tile
        chan_in_buf_word(c_pcm_in, (uint32_t*)&pipeline_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
#endif
        
        // Create interleaved output that can be written to wav file
        for (unsigned ch=0;ch<AP_MAX_Y_CHANNELS;ch++){
            for(unsigned i=0;i<AP_FRAME_ADVANCE;i++){
                output_write_buffer[i*(AP_MAX_Y_CHANNELS) + ch] = pipeline_output[ch][i];
            }
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * AP_FRAME_ADVANCE * AP_MAX_Y_CHANNELS);
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if !X86_BUILD
void pipeline_wrapper_tile1(chanend_t c_pcm_in, chanend_t c_pcm_out)
{
    pipeline_state_tile1_t DWORD_ALIGNED pipeline_tile1_state;
    pipeline_metadata_t md;
    int32_t DWORD_ALIGNED tile0_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED pipeline_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    pipeline_tile1_init(&pipeline_tile1_state);
    while(1) {
        chan_in_buf_byte(c_pcm_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_in_buf_word(c_pcm_in, (uint32_t*)&tile0_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        pipeline_process_frame_tile1(&pipeline_tile1_state, &md, tile0_output, pipeline_output);

        chan_out_buf_word(c_pcm_out, (uint32_t*)&pipeline_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
    }
}
#endif

#if X86_BUILD
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    pipeline_wrapper_tile0(0, 0, argv[1], argv[2]);
    return 0;
}
#endif
