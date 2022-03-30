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

void print_output()
{
  for (int i = 0; i < output_size; i++)
  {
    printf("Output index=%u, value=%i\n", i, (signed char)output_buffer[i]);
  }
  printf("DONE!\n");
}

void run_inference()
{
    
    //uint64_t start = (uint64_t)get_reference_time();
    interp_invoke(&ie);
    //uint64_t end = (uint64_t)get_reference_time();
    //printf("cycles = %lld\n",end-start);
    print_profiler_summary(&ie);

    print_output();
}


void vnr_task(const char *input_file_name, const char *output_file_name){
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
  // run inference with an all zero input tensor as a check
  memset(input_buffer, input_size, 0);
  run_inference();
  exit(0);
}
#ifdef __cplusplus
};
#endif
