// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr.h"

static vnr_state_t vnr;

void test_init()
{
    vnr_state_init(&vnr);
}

void test(int32_t *output, int32_t *input)
{
    vnr.feature_state.config.enable_highpass = input[VNR_FRAME_ADVANCE]; // Highpass enabled flag sent as the last value

    vnr_process_frame(&vnr, (float_s32_t*)output, input);
}

