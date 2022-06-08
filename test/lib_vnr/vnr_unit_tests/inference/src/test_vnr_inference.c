// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr_features_state.h" // For VNR_PATCH_WIDTH and VNR_MEL_FILTERS defines
#include "vnr_inference_api.h"

static vnr_ie_state_t ie_state;

void test_init()
{
    int32_t ret = vnr_inference_init(&ie_state);
    if(ret) {
        printf("vnr_inference_init() returned error %ld\n",ret);
        assert(0);
    }
}

void test(int32_t *output, int32_t *input)
{
    bfp_s32_t this_patch;
    // input has exponent followed by 96 data values
    bfp_s32_init(&this_patch, &input[1], input[0], VNR_PATCH_WIDTH*VNR_MEL_FILTERS, 1);

    vnr_inference(&ie_state, (float_s32_t*)output, &this_patch);
}
