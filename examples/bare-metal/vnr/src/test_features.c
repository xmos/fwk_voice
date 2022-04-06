// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xcore/hwtimer.h>
#include "vnr_features_api.h"
#include "fileio.h"
#include "wav_utils.h"

void test_features(const char *input_wav_file, const char *output_features_file, const char *output_exp_file) {

    file_t input_file, feature_file, exp_file;

    int ret = file_open(&input_file, input_wav_file, "rb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&feature_file, output_features_file, "wb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&exp_file, output_exp_file, "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 16bit wav file
    if(input_header_struct.bit_depth != 16){
        printf("Test works on only 16 bit wav files. %d bit wav input provided\n",input_header_struct.bit_depth);
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

    vnr_input_state_t DWORD_ALIGNED vnr_input_state;
    vnr_input_state_init(&vnr_input_state);

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);
    int16_t input_read_buffer[VNR_FRAME_ADVANCE] = {0}; // Array for storing interleaved input read from wav file
    int32_t new_frame[VNR_FRAME_ADVANCE] = {0};

    complex_s32_t debug_output[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t debug_bfp;
    bfp_complex_s32_init(&debug_bfp, debug_output, 0, VNR_FD_FRAME_LENGTH, 0);

    for(unsigned b=0; b<block_count; b++) {
        //printf("frame %d\n",b);
        long input_location =  wav_get_frame_start(&input_header_struct, b * VNR_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* VNR_FRAME_ADVANCE);
        // Convert to 32 bit
        for(int i=0; i<VNR_FRAME_ADVANCE; i++) {
            new_frame[i] = (int32_t)input_read_buffer[i] << 16; //1.31
        }
        vnr_extract_features(&vnr_input_state, new_frame, &debug_bfp);
        
        file_write(&feature_file, (uint8_t*)(debug_bfp.data), debug_bfp.length*sizeof(complex_s32_t));
        file_write(&exp_file, (uint8_t*)(&debug_bfp.exp), 1*sizeof(exponent_t));
    }
}
