// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xcore/hwtimer.h>
#include "vnr_inference_api.h"

static vnr_ie_t vnr_ie_state;


#ifdef __cplusplus
extern "C" {
#endif
#include "fileio.h"
#include "wav_utils.h"

static uint32_t max_cycles = 0;
static uint64_t sum_cycles = 0;
static int frames = 0;
static uint32_t mean_cycles = 0;
int8_t run_inference(vnr_ie_t *ie_state)
{
    int8_t inference_output;
    frames++;
    uint64_t start = (uint64_t)get_reference_time();

    vnr_inference(&vnr_ie_state, &inference_output);

    uint64_t end = (uint64_t)get_reference_time();
    //printf("cycles = %lld\n",end-start);
    uint32_t cycles = (end - start);
    if(cycles > max_cycles) {max_cycles = cycles;}
    sum_cycles = mean_cycles*(frames-1) + cycles;
    mean_cycles = sum_cycles/frames;
    //print_profiler_summary(&ie);

    return inference_output;
}

void vnr_task(const char *feature_in_filename, const char *feature_out_filename, const char *inference_out_filename)
{
    vnr_inference_init(&vnr_ie_state);

    file_t features_file, features_loopback_file, inference_file;
    int ret = file_open(&features_file, feature_in_filename, "rb");
    ret = file_open(&features_loopback_file, feature_out_filename, "wb");
    ret = file_open(&inference_file, inference_out_filename, "wb");

    int file_size = get_file_size(&features_file);
    int num_frames = file_size/vnr_ie_state.input_size;
    printf("%d Frames in this test stream\n",num_frames);
    
    for(unsigned fr=0; fr<num_frames; fr++) {
        file_read(&features_file, (uint8_t*)vnr_ie_state.input_buffer, vnr_ie_state.input_size);

        int8_t output = run_inference(&vnr_ie_state);

        file_write(&inference_file, (uint8_t*)&output, 1);

        file_write(&features_loopback_file, (uint8_t*)vnr_ie_state.input_buffer, vnr_ie_state.input_size);
    }
    printf("Inference call: Mean cycles %d, Max cycles %d\n",mean_cycles, max_cycles);
    shutdown_session();

    // run inference with an all zero input tensor as a check
    //memset(input_buffer, input_size, 0);
    //run_inference();
    //exit(0);
}
#ifdef __cplusplus
};
#endif
