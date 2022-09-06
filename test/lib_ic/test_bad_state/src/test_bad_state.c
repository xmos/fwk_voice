// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "ic_api.h"

static ic_state_t DWORD_ALIGNED ic_state;

void test_init(int32_t conf, int32_t * H_data)
{
    ic_init(&ic_state);
    ic_state.ic_adaption_controller_state.adaption_controller_config.adaption_config = conf;
    int indx = 0;
    for(int ph = 0; ph < IC_FILTER_PHASES; ph++){
	// Python forms data in q29 format to fill some bigger values
        ic_state.H_hat_bfp[0][ph].exp = -29;
        memcpy(&ic_state.H_hat[0][ph][0], &H_data[indx], IC_FD_FRAME_LENGTH * sizeof(int32_t));
        ic_state.H_hat_bfp[0][ph].hr = bfp_complex_s32_headroom(&ic_state.H_hat_bfp[0][ph]);
        indx += IC_FD_FRAME_LENGTH;
    }
}

void test(int32_t * output, int32_t * y_frame, int32_t * x_frame)
{
    float_s32_t input_vnr_pred, output_vnr_pred;
    ic_filter(&ic_state, y_frame, x_frame, output);
    ic_calc_vnr_pred(&ic_state, &input_vnr_pred, &output_vnr_pred);
    ic_adapt(&ic_state, input_vnr_pred);
}
