// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr_defines.h"
#include "vnr_inference_api.h"
#include "vnr_inference_priv.h"

static vnr_model_quant_spec_t vnr_quant_spec;

void test_init()
{
    vnr_priv_init_quant_spec(&vnr_quant_spec);
}

void test(int32_t *output, int32_t *input)
{
    bfp_s32_t this_patch;
    // input has exponent followed by 96 data values
    bfp_s32_init(&this_patch, &input[1], input[0], VNR_PATCH_WIDTH*VNR_MEL_FILTERS, 1);
    vnr_priv_feature_quantise((int8_t*)output, &this_patch, &vnr_quant_spec);
}
