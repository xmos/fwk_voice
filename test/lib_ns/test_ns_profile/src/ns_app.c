// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <ns.h>

#include <fileio.h>

#if PROFILE_PROCESSING
#include "profile.h"
#else
static void prof(int n, const char* str) {}
static void print_prof(int a, int b, int framenum){}
#endif

extern void ns_process_frame(ns_state_t * state,
                        int32_t output [NS_FRAME_ADVANCE],
                        const int32_t input[NS_FRAME_ADVANCE]);

void ns_task(const char *input_file_name, const char *output_file_name){
    file_t input_file, output_file;

    // Open binary files for input and output of the NS
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    int32_t file_size = get_file_size(&input_file);
    unsigned frame_count = file_size / sizeof(int32_t);
    unsigned block_count = frame_count / NS_FRAME_ADVANCE;

    int32_t DWORD_ALIGNED frame[NS_FRAME_ADVANCE] = {0};

    //Initialise noise suppressor
    prof(0, "start_ns_init");

    ns_state_t DWORD_ALIGNED ch1_state;

    ns_init(&ch1_state);

    prof(1, "end_ns_init");

    for(int b = 0; b < block_count; b++){
        file_read (&input_file, (uint8_t*)&frame[0], sizeof(int32_t) * NS_FRAME_ADVANCE);
        // Call Noise Suppression functions to process NS_FRAME_ADVANCE new samples of data
        // Reuse mic data memory for main filter output
        prof(2, "start_ns_process_frame");

        ns_process_frame(&ch1_state, frame, frame);

        prof(3, "end_ns_process_frame");

        file_write(&output_file, (uint8_t*)frame, sizeof(int32_t) * NS_FRAME_ADVANCE);

        print_prof(0, 4, b+1);
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if X86_BUILD
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    ns_task(argv[1], argv[2]);
    return 0;
}
#endif
