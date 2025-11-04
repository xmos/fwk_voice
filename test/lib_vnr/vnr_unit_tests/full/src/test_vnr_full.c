// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr.h"

void test_init(){
    // intentionally empty
}

void test(int32_t *output, int32_t *input)
{
    vnr_ctx_t vnr_ctx;
    vnr_init(&vnr_ctx);
    vnr_ctx.vnr_feature_state.config.enable_highpass = input[VNR_FRAME_ADVANCE]; // Highpass enabled flag sent as the last value
    vnr_add_frame(&vnr_ctx, input);
    vnr_compute(&vnr_ctx);
    memcpy(output, &vnr_ctx.vnr_output_s32, sizeof(float_s32_t));
}

