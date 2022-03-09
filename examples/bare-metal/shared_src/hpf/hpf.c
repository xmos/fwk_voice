// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf.h"

// Q30 coefficients for the 100 Hz high pass filter
const fixed_s32_t hpf_coef_q30[10] = {
    1020035168, -2040070348, 1020035168, 2070720224, -998576072,
    1073741824, -2147483648, 1073741824, 2114066120, -1041955416
};

void pre_agc_hpf(int32_t DWORD_ALIGNED data[240]){
    xs3_biquad_filter_s32_t filter = {0};
    filter.biquad_count = 2;
    int i = 0, j = 0;
    for(int v = 0; v < 10; v++){
        if(i == 5){i = 0; j++;}
        filter.coef[i][j] = hpf_coef_q30[v];
        i++;
    }
    for(int v = 0; v < 240; v++) {
        // shifting one bit right to match the coeficients format 
        // the output will be in Q30, so we need to shift it one bit left
        data[v] = xs3_filter_biquad_s32(&filter, data[v] >> 1);
    }
    // using bfp because it has saturation logic
    bfp_s32_t bfp;
    bfp_s32_init(&bfp, data, -30, 240, 1);
    bfp_s32_use_exponent(&bfp, -31);
}
