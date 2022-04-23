// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "vnr_inference_api.h"

static struct tflite_micro_objects tflmo;

void vnr_inference_init(vnr_ie_t *ie_ptr) {
    auto *resolver = inference_engine_initialize(&ie_ptr->ie, (uint32_t *)&ie_ptr->tensor_arena, TENSOR_ARENA_SIZE_BYTES, (uint32_t *) vnr_model_data, vnr_model_data_len, &tflmo);
    resolver->AddConv2D();
    resolver->AddReshape();
    resolver->AddLogistic();
    resolver->AddCustom(tflite::ops::micro::xcore::Conv2D_V2_OpCode,
            tflite::ops::micro::xcore::Register_Conv2D_V2());
    inference_engine_load_model(&ie_ptr->ie, vnr_model_data_len, (uint32_t *) vnr_model_data, 0);
    
    ie_ptr->input_buffer = (int8_t *) ie_ptr->ie.input_buffers[0];
    ie_ptr->input_size = ie_ptr->ie.input_sizes[0];
    ie_ptr->output_buffer = (int8_t *) ie_ptr->ie.output_buffers[0];
    ie_ptr->output_size = ie_ptr->ie.output_sizes[0];   
    printf("input_size %d, output_size %d\n", ie_ptr->input_size, ie_ptr->output_size);
}


void vnr_inference(vnr_ie_t *ie_state, int8_t *vnr_output, const int8_t *features) {
    memcpy(ie_state->input_buffer, features, ie_state->input_size);
    interp_invoke(&ie_state->ie);
    memcpy(vnr_output, ie_state->output_buffer, ie_state->output_size*sizeof(int8_t));
}

