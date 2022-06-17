// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr_defines.h"
#include "vnr_inference_priv.h"

static vnr_model_quant_spec_t vnr_quant_spec;

void test_init()
{
    vnr_priv_init_quant_spec(&vnr_quant_spec);
}

void test(int32_t *output, int32_t *input)
{
    vnr_priv_output_dequantise((float_s32_t*)output, (int8_t*)input, &vnr_quant_spec);
}
