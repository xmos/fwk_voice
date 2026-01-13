// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#if !X86_BUILD
#ifdef __XC__
    #define chanend_t chanend
#else
    #include <xcore/chanend.h>
#endif
#include <platform.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "ic.h"
#include "fileio.h"

#define INPUT_CHANNELS (IC_Y_CHANNELS + IC_X_CHANNELS)

#if INPUT_CHANNELS != 2
#error "Number of INPUT_CHANNELS has to be 2"
#endif

#if PROFILE_PROCESSING
#include "profile.h"
#endif

void ic_task(const char *input_file_name, const char *output_file_name) {
    //open files
    file_t input_file, output_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    int file_size = get_file_size(&input_file);
    unsigned frame_count = file_size / (sizeof(int32_t) * INPUT_CHANNELS);
    unsigned block_count = frame_count / IC_FRAME_ADVANCE;

    int32_t DWORD_ALIGNED frame_y[IC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED frame_x[IC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED output[IC_FRAME_ADVANCE];

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
        // Python makes sure that, Y and X frames interleaved accordingly
        file_read(&input_file, (uint8_t*)frame_y, sizeof(int32_t) * IC_FRAME_ADVANCE);
        file_read(&input_file, (uint8_t*)frame_x, sizeof(int32_t) * IC_FRAME_ADVANCE);

        prof(2, "start_ic_filter");
        // Call IC functions to process IC_FRAME_ADVANCE new samples of data
        ic_filter(&state,  frame_y, frame_x, output);
        prof(3, "end_ic_filter");

        prof(4, "start_vad_estimate");
        float_s32_t vnr = {0,0};
        (void)vnr;
        prof(5, "end_vad_estimate");

        prof(6, "start_ic_adapt");
        ic_adapt(&state);
        prof(7, "end_ic_adapt");

        file_write(&output_file, (uint8_t*)output, sizeof(int32_t) * IC_FRAME_ADVANCE);
        print_prof(0,8,b+1);
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}


#if !X86_BUILD
void main_tile1(chanend_t c_cross_tile)
{
    //Do nothing
}

#define IN_WAV_FILE_NAME    "input.bin"
#define OUT_WAV_FILE_NAME   "output.bin"
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
