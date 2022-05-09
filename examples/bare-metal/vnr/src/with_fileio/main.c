// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/select.h>
#include <xcore/hwtimer.h>
#include <xscope.h>
#include <stdlib.h>
#include <string.h>
#include "xs3_math.h"
#include "fileio.h"
#include "wav_utils.h"
#include "vnr_features_api.h"



void get_frame(chanend_t c_get_frame, const char* in_filename)
{
    hwtimer_t timer = hwtimer_alloc();
    file_t input_file;
    int ret = file_open(&input_file, in_filename, "rb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 16bit wav file
    if((input_header_struct.bit_depth != 32) && (input_header_struct.bit_depth != 16)){
        printf("Test works on only 16 bit or 32 bit wav files. %d bit wav input provided\n",input_header_struct.bit_depth);
        assert(0);
    }

    // Ensure input wav file contains correct number of channels 
    if(input_header_struct.num_channels != 1){
        printf("Error: wav num channels(%d) does not match expected 1 channel\n", input_header_struct.num_channels);
        _Exit(1);
    }
    
    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / VNR_FRAME_ADVANCE;
    printf("%d Frames in this test input wav\n",block_count);
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    int16_t input_read_buffer[VNR_FRAME_ADVANCE] = {0}; // Array for storing interleaved input read from wav file
    int32_t new_frame[VNR_FRAME_ADVANCE] = {0};

    // Send frame count over to receive thread
    
    for(unsigned b=0; b<block_count; b++) {
        long input_location =  wav_get_frame_start(&input_header_struct, b * VNR_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        
        if(input_header_struct.bit_depth == 16){
            file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* VNR_FRAME_ADVANCE);
            // Convert to 32 bit
            for(int i=0; i<VNR_FRAME_ADVANCE; i++) {
                new_frame[i] = (int32_t)input_read_buffer[i] << 16; //1.31
            }
        }
        else {
            file_read (&input_file, (uint8_t*)&new_frame[0], bytes_per_frame* VNR_FRAME_ADVANCE);
        }
        // Transmit input frame over channel
        chan_out_buf_word(c_get_frame, (uint32_t*)&new_frame[0], VNR_FRAME_ADVANCE);                
    }
    // Sleep to allow last frame processing
    hwtimer_delay(timer, 10000000); //100 ms delay to allow last frame to get written out from send_frame()
    shutdown_session();
}

void send_frame(chanend_t c_send_frame, const char* out_filename)
{
    file_t output_file;
    int ret = file_open(&output_file, out_filename, "wb");
    assert((!ret) && "Failed to open file");

    while(1) {
        float_s32_t vnr_out;
        chan_in_buf_byte(c_send_frame, (uint8_t*)&vnr_out, sizeof(float_s32_t));
        file_write(&output_file, (uint8_t*)&vnr_out, sizeof(float_s32_t));
    }
    
}
