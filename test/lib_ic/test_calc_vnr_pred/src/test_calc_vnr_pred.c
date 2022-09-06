// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "ic_api.h"

static ic_state_t DWORD_ALIGNED ic_state;
void test_init()
{
    ic_init(&ic_state);
}

void test(int32_t *output, int32_t *input)
{
    // Read input buffer
    //input contains Y exp, followed by Y bfp complex data followed by Error exp followed by Error bfp complex data
    // Update Y_bfp
    ic_state.Y_bfp[0].exp = input[0];
    memcpy(ic_state.Y_bfp[0].data, &input[1], ic_state.Y_bfp[0].length*sizeof(complex_s32_t));
    ic_state.Y_bfp[0].hr = bfp_complex_s32_headroom(&ic_state.Y_bfp[0]);

    // Update Error_bfp
    ic_state.Error_bfp[0].exp = input[(1 + 2*ic_state.Y_bfp[0].length)];
    memcpy(ic_state.Error_bfp[0].data, &input[1 + (1 + 2*ic_state.Y_bfp[0].length)], ic_state.Error_bfp[0].length*sizeof(complex_s32_t));
    ic_state.Error_bfp[0].hr = bfp_complex_s32_headroom(&ic_state.Error_bfp[0]);

    float_s32_t input_vnr_pred, output_vnr_pred;
    ic_calc_vnr_pred(&ic_state, &input_vnr_pred, &output_vnr_pred);
    // Write to output buffer
    memcpy(&output[0], &input_vnr_pred, sizeof(float_s32_t));
    memcpy(&output[2], &output_vnr_pred, sizeof(float_s32_t));
}

