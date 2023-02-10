// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf.h"

// Q30 coefficients for the 100 Hz high pass filter
// coefficients were generated here:
// https://github.com/xmos/lib_audio_pipelines/blob/develop/lib_audio_pipelines/src/dsp/ap_stage_c.xc#L26
const q2_30 hpf_coef_q30[TOTAL_NUM_COEFF] = {
    1020035168, -2040070348, 1020035168, 2070720224, -998576072,
    1073741824, -2147483648, 1073741824, 2114066120, -1041955416
};

void pre_agc_hpf(int32_t DWORD_ALIGNED data[AP_FRAME_ADVANCE]){
    filter_biquad_s32_t filter = {0};
    filter.biquad_count = BIQUAD_COUNT;
    int i = 0, j = 0;
    for(int v = 0; v < TOTAL_NUM_COEFF; v++){
        if(i == NUM_COEFF_PER_BIQUAD){i = 0; j++;}
        filter.coef[i][j] = hpf_coef_q30[v];
        i++;
    }
    for(int v = 0; v < AP_FRAME_ADVANCE; v++) {
        // shifting one bit right to match the coeficients format 
        // the output will be in Q30, so we need to shift it one bit left
        data[v] = filter_biquad_s32(&filter, data[v] >> 1);
    }
    // using bfp because it has saturation logic
    bfp_s32_t bfp;
    bfp_s32_init(&bfp, data, FILT_EXP, AP_FRAME_ADVANCE, 1);
    bfp_s32_use_exponent(&bfp, NORM_EXP);
}
