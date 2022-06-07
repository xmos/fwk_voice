// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "inference_engine.h"
#include "vnr_inference_api.h"
#include "vnr_inference_priv.h"

static struct tflite_micro_objects tflmo;
static inference_engine_t ie; 

// TODO: unsure why the stack can not be computed automatically here
#pragma stackfunction 1000
int32_t vnr_inference_init(vnr_ie_state_t *ie_ptr) {
    auto *resolver = inference_engine_initialize(&ie, (uint32_t *)&ie_ptr->tensor_arena, TENSOR_ARENA_SIZE_BYTES, (uint32_t *) vnr_model_data, vnr_model_data_len, &tflmo);
    // When adding a new operator, refer to lib_tflite_micro/lib_tflite_micro/submodules/tflite-micro/tensorflow/lite/micro/all_ops_resolver.cc for all available add operator functions
    resolver->AddConv2D();
    resolver->AddReshape();
    resolver->AddLogistic();
    resolver->AddCustom(tflite::ops::micro::xcore::Conv2D_V2_OpCode,
            tflite::ops::micro::xcore::Register_Conv2D_V2());
    int ret = inference_engine_load_model(&ie, vnr_model_data_len, (uint32_t *) vnr_model_data, 0);

    //printf("ret from inference_engine_load_model = %d, memory primary bytes = %d, %d\n",ret, ie.memory_primary_bytes, ie.xtflm->interpreter->arena_used_bytes());

    
    ie_ptr->input_buffer = (int8_t *) ie.input_buffers[0];
    ie_ptr->input_size = ie.input_sizes[0];
    ie_ptr->output_buffer = (int8_t *) ie.output_buffers[0];
    ie_ptr->output_size = ie.output_sizes[0];   
    
    // Initialise input quant and output dequant parameters
    vnr_priv_init_ie_config(&ie_ptr->ie_config);
    return ret;
}



#pragma stackfunction 1000
void vnr_inference(vnr_ie_state_t *ie_state, float_s32_t *vnr_output, bfp_s32_t *features) {
    // Quantise features to 8bit
    vnr_priv_feature_quantise(ie_state->input_buffer, features, &ie_state->ie_config);
    
    // Inference
    interp_invoke(&ie);

    // Dequantise inference output
    vnr_priv_output_dequantise(vnr_output, ie_state->output_buffer, &ie_state->ie_config);
}

