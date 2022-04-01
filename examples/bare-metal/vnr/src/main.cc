// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "xtflm_conf.h"
#include "inference_engine.h"
#include "vnr_model_data.h"
#include <xcore/hwtimer.h>


#define TENSOR_ARENA_SIZE_BYTES 100000
static uint64_t tensor_arena[TENSOR_ARENA_SIZE_BYTES/sizeof(uint64_t)];

static inference_engine_t ie;
static struct tflite_micro_objects tflmo;

static int8_t *input_buffer;
static size_t input_size;
static int8_t *output_buffer;
static size_t output_size;
static int input_bytes = 0;

#ifdef __cplusplus
extern "C" {
#endif
#include "fileio.h"
#include "wav_utils.h"
void print_output()
{
  for (int i = 0; i < output_size; i++)
  {
    printf("Output index=%u, value=%i\n", i, (signed char)output_buffer[i]);
  }
  printf("DONE!\n");
}

static uint32_t max_cycles = 0;
static uint64_t sum_cycles = 0;
static int frames = 0;
static uint32_t mean_cycles = 0;
void run_inference()
{
    frames++;
    uint64_t start = (uint64_t)get_reference_time();
    interp_invoke(&ie);
    uint64_t end = (uint64_t)get_reference_time();
    //printf("cycles = %lld\n",end-start);
    uint32_t cycles = (end - start);
    if(cycles > max_cycles) {max_cycles = cycles;}
    sum_cycles = mean_cycles*(frames-1) + cycles;
    mean_cycles = sum_cycles/frames;
    //print_profiler_summary(&ie);

    //print_output();
}

void vnr_task(const char *feature_in_filename, const char *feature_out_filename, const char *inference_out_filename){
    auto *resolver = inference_engine_initialize(&ie, (uint32_t *)tensor_arena, TENSOR_ARENA_SIZE_BYTES, (uint32_t *) vnr_model_data, vnr_model_data_len, &tflmo);
    resolver->AddQuantize();
    resolver->AddConv2D();
    resolver->AddRelu();
    resolver->AddReshape();
    resolver->AddLogistic();
    resolver->AddDequantize();
    resolver->AddCustom(tflite::ops::micro::xcore::Conv2D_V2_OpCode,
            tflite::ops::micro::xcore::Register_Conv2D_V2());
    inference_engine_load_model(&ie, vnr_model_data_len, (uint32_t *) vnr_model_data, 0);

    input_buffer = (int8_t *) ie.input_buffers[0];
    input_size = ie.input_sizes[0];
    output_buffer = (int8_t *) ie.output_buffers[0];
    output_size = ie.output_sizes[0];   

    printf("input_size %d, output_size %d\n",input_size, output_size);

    file_t features_file, features_loopback_file, inference_file;
    int ret = file_open(&features_file, feature_in_filename, "rb");
    ret = file_open(&features_loopback_file, feature_out_filename, "wb");
    ret = file_open(&inference_file, inference_out_filename, "wb");


    int file_size = get_file_size(&features_file);
    int num_frames = file_size/input_size;

    printf("%d Frames in this test stream\n",num_frames);
    
    char strbuf[100];
    for(unsigned fr=0; fr<num_frames; fr++) {
        file_read(&features_file, (uint8_t*)input_buffer, input_size);
        run_inference();
        file_write(&inference_file, (uint8_t*)output_buffer, output_size);
        file_write(&features_loopback_file, (uint8_t*)input_buffer, input_size);
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
