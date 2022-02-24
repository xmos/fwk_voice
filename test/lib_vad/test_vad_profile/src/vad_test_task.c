// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/chanend.h>
#include <platform.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vad_api.h"

#include "fileio.h"
#include "wav_utils.h"
#include "xs3_math.h"
#include "profile.h"


void vad_task(const char *input_file_name) {
    //open files
    file_t input_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct;
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

    if(input_header_struct.num_channels != 1){
        printf("Error: wav num channels(%d) does not match ic(%u)\n", input_header_struct.num_channels, 1);
        _Exit(1);
    }
    

    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    unsigned block_count = frame_count / VAD_FRAME_ADVANCE;

#if MAX_FRAMES
    if(block_count > MAX_FRAMES){
        block_count = MAX_FRAMES;
    }
#endif

    int32_t input_read_buffer[VAD_FRAME_ADVANCE * 1] = {0};
    int32_t DWORD_ALIGNED input[VAD_FRAME_ADVANCE];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    //Start ic
    vad_state_t state;
    prof(0, "start_ic_init");
    vad_init(&state);
    prof(1, "end_ic_init");


    for(unsigned b=0;b<block_count;b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * VAD_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame * VAD_FRAME_ADVANCE);
        for(unsigned f=0; f<VAD_FRAME_ADVANCE; f++){
            input[f] = input_read_buffer[f];
        }

        prof(2, "start_vad");
        vad_probability_voice(input, &state);
        prof(3, "end_vad");
        print_prof(0,4,b+1);

    }
    file_close(&input_file);
    shutdown_session();
}


void main_tile1(chanend_t c_cross_tile)
{
    //Do nothing
}

#define IN_WAV_FILE_NAME    "input.wav"
void main_tile0(chanend_t c_cross_tile, chanend_t xscope_chan)
{
    xscope_io_init(xscope_chan);
    vad_task(IN_WAV_FILE_NAME);
}
