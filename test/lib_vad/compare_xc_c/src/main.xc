// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <string.h>

#include "vad.h" 

extern "C"{
    #include "fileio.h"
    #include "wav_utils.h"
}
#include <platform.h>
#include <xs1.h>
#include <xscope.h>
#include "xscope_io_device.h"

#define IN_WAV_FILE_NAME    "input.wav"

void vad_task(const char *input_file_name) {

    printf("here\n");

    file_t input_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");

    printf("here\n");

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
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);


    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    unsigned block_count = frame_count / VAD_FRAME_ADVANCE;

    printf("here\n");

    vad_state_t [[aligned(8)]] state;
    vad_init_state(state);


    int32_t input_samples[VAD_FRAME_ADVANCE] = {0};
    int32_t [[aligned(8)]] input_frame[VAD_PROC_FRAME_LENGTH] = {0};

    printf("here\n");

    for(unsigned b=0;b<block_count;b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * VAD_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_samples[0], bytes_per_frame * VAD_FRAME_ADVANCE);

        memmove(&input_frame[0], &input_frame[VAD_FRAME_ADVANCE], (VAD_PROC_FRAME_LENGTH-VAD_FRAME_ADVANCE) * sizeof(int32_t));
        memcpy(&input_frame[VAD_PROC_FRAME_LENGTH-VAD_FRAME_ADVANCE], &input_samples[0], VAD_FRAME_ADVANCE * sizeof(int32_t));

        int32_t vad_percentage = vad_percentage_voice(input_frame, state);
        printf("VAD frame: %d result: %d\n", b, vad_percentage);

    }
    file_close(&input_file);
    shutdown_session();
}


void main_tile1(chanend c_cross_tile)
{
    //Do nothing
}

#define IN_WAV_FILE_NAME    "input.wav"
void main_tile0(chanend c_cross_tile, chanend xscope_chan)
{
    xscope_io_init(xscope_chan);
    vad_task(IN_WAV_FILE_NAME);
}

int main(void)
{
  chan c_cross_tile, xscope_chan;
  par
  {
    xscope_host_data(xscope_chan);
    on tile[0]: {
        main_tile0(c_cross_tile, xscope_chan);
        _Exit(0);
    }
    on tile[1]: main_tile1(c_cross_tile);
  }
  return 0;
}
