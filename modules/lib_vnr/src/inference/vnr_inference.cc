// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "wrapper.h"
#include "vnr_inference_api.h"
#include "vnr_inference_priv.h"

// Allocate all memory required by the inference engine
static vnr_model_quant_spec_t vnr_quant_state;


// TODO: unsure why the stack can not be computed automatically here
#pragma stackfunction 1000
int32_t vnr_inference_init() {

    int32_t ret = vnr_init();

    // Initialise input quant and output dequant parameters
    vnr_priv_init_quant_spec(&vnr_quant_state);
    return ret;
}

#pragma stackfunction 1000
void vnr_inference(float_s32_t *vnr_output, bfp_s32_t *features) {
    int8_t * in_buffer = vnr_get_input();
    int8_t * out_buffer = vnr_get_output();
    // Quantise features to 8bit
    vnr_priv_feature_quantise(in_buffer, features, &vnr_quant_state);

    // Inference
    vnr_inference_invoke();

    // Dequantise inference output
    vnr_priv_output_dequantise(vnr_output, out_buffer, &vnr_quant_state);
}

