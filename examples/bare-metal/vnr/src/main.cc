// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "xtflm_conf.h"
#include "inference_engine.h"
#include "vnr_model_data.h"

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
    
}
#ifdef __cplusplus
};
#endif
