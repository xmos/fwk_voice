#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/channel_transaction.h>
#include <xcore/port.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>
#include "xmath/xmath.h"
#include "fileio.h"
#include "wav_utils.h"

#include "pipeline_config.h"
#include "pipeline_state.h"

DECLARE_JOB(tx, (chanend_t, chanend_t, const char*));
DECLARE_JOB(pipeline_tile0, (chanend_t, chanend_t));
DECLARE_JOB(rx, (chanend_t, chanend_t, const char*));

extern void pipeline_tile1(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a);

/// tx
void tx(chanend_t c_pcm_in_a, chanend_t c_wavheader_a, const char* input_file_name) {
    file_t input_file;
    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
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
    
    // Send output_header_struct to the rx thread
    chan_out_buf_byte(c_wavheader_a, (uint8_t*)&output_header_struct, sizeof(wav_header));


    int32_t input_read_buffer[AP_FRAME_ADVANCE * (AP_MAX_Y_CHANNELS + AP_MAX_X_CHANNELS)] = {0}; // Array for storing interleaved input read from wav file
    int32_t DWORD_ALIGNED frame[AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    for(unsigned b=0;b<block_count;b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * AP_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* AP_FRAME_ADVANCE);
        // Deinterleave and copy to a [channels][240] array
        for(unsigned f=0; f<AP_FRAME_ADVANCE; f++){
            for(unsigned ch=0; ch<(AP_MAX_Y_CHANNELS + AP_MAX_Y_CHANNELS); ch++){
                unsigned i =(f * (AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS)) + ch;
                frame[ch][f] = input_read_buffer[i];
            }
        }
        // Transmit input frame over channel
        chan_out_buf_word(c_pcm_in_a, (uint32_t*)&frame[0][0], ((AP_MAX_Y_CHANNELS+AP_MAX_X_CHANNELS) * AP_FRAME_ADVANCE));
    }
}

/// rx
void rx(chanend_t c_pcm_out_b, chanend_t c_wavheader_b, const char* output_file_name) {
    file_t output_file;
    int32_t output_write_buffer[AP_FRAME_ADVANCE * (AP_MAX_Y_CHANNELS)];
    int32_t DWORD_ALIGNED pipeline_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    int ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");
    wav_header output_header_struct;
    // Wait for output header to be sent to us
    chan_in_buf_byte(c_wavheader_b, (uint8_t*)&output_header_struct, sizeof(wav_header));

    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);
    
    int32_t num_frames = (output_header_struct.data_bytes) / (output_header_struct.num_channels * (output_header_struct.bit_depth/8) * AP_FRAME_ADVANCE);
    for(int frame=0; frame<num_frames; frame++)
    {
        // Receive output frame over channel
        chan_in_buf_word(c_pcm_out_b, (uint32_t*)&pipeline_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        // Create interleaved output that can be written to wav file
        for (unsigned ch=0;ch<AP_MAX_Y_CHANNELS;ch++){
            for(unsigned i=0;i<AP_FRAME_ADVANCE;i++){
                output_write_buffer[i*(AP_MAX_Y_CHANNELS) + ch] = pipeline_output[ch][i];
            }
        }
        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * AP_FRAME_ADVANCE * AP_MAX_Y_CHANNELS);
    }
    
    shutdown_session();
    _Exit(0);
}

//**** Multi tile pipeline structure ***//
// file_read -> stage1 -> (tile0_to_tile1)-> stage2 -> stage3 -> stage4 -> (tile1_to_tile0) -> file_write
void main_tile0(chanend_t c_t0_t1, chanend_t c_t1_t0, const char *input_file_name, const char* output_file_name)
{
    channel_t c_pcm_in = chan_alloc();
    channel_t c_wavheader = chan_alloc();
    PAR_JOBS(
        PJOB(tx, (c_pcm_in.end_a, c_wavheader.end_a, input_file_name)),
        PJOB(pipeline_tile0, (c_pcm_in.end_b, c_t0_t1)),
        PJOB(rx, (c_t1_t0, c_wavheader.end_b, output_file_name))
        );
}

void main_tile1(chanend_t c_t0_t1, chanend_t c_t1_t0)
{
    pipeline_tile1(c_t0_t1, c_t1_t0);
}

