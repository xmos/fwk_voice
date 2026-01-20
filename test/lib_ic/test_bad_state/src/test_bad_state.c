// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "ic.h"

static ic_state_t DWORD_ALIGNED ic_state;

void test_init(int32_t conf, int32_t * H_data)
{
    ic_init(&ic_state);
    ic_state.ic_adaption_controller_state.adaption_controller_config.adaption_config = conf;
    ic_state.ic_adaption_controller_state.adaption_controller_config.enable_adaption = 1;
    ic_state.config_params.bypass = 0;

    // Set leakage_alpha to 1.0 for FORCE_OFF mode to prevent decay
    if(conf == IC_ADAPTION_FORCE_OFF) {
        ic_state.leakage_alpha.mant = (1 << 30);  // 1.0 in Q30 format
        ic_state.leakage_alpha.exp = -30;
    }

    int indx = 0;
    for(int ph = 0; ph < IC_FILTER_PHASES; ph++){
        // Copy the filter data directly
        memcpy(&ic_state.H_hat[0][ph][0], &H_data[indx], IC_FD_FRAME_LENGTH * sizeof(complex_s32_t));
        // Reinitialize BFP with the new data - this will recalculate everything properly
        bfp_complex_s32_init(&ic_state.H_hat_bfp[0][ph], &ic_state.H_hat[0][ph][0], -29, IC_FD_FRAME_LENGTH, 1);
        indx += IC_FD_FRAME_LENGTH * 2;
    }
}

void test(int32_t * output, int32_t * y_frame, int32_t * x_frame)
{
    float_s32_t input_vnr_pred;
    ic_process_frame(&ic_state, y_frame, x_frame, output, &input_vnr_pred);
}
