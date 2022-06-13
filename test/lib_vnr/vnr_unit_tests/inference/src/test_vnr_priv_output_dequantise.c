// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr_features_state.h" // For VNR_PATCH_WIDTH and VNR_MEL_FILTERS defines
#include "vnr_inference_api.h"
#include "vnr_inference_priv.h"

static vnr_ie_state_t ie_state;

void test_init()
{
    vnr_priv_init_ie_config(&ie_state.ie_config);
}

void test(int32_t *output, int32_t *input)
{
    vnr_priv_output_dequantise((float_s32_t*)output, (int8_t*)input, &ie_state.ie_config);
}
