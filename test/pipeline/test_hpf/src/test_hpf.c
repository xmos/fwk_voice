// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf_test.h"

#define len 512

// 100Hz HPF
int32_t stage_c_filt_coeffs[10] = {
    Q28(+0.94998178), Q28(-1.89996357), Q28(+0.94998178), Q28(+1.92850849), Q28(-0.92999644),
    Q28(+1.0),        Q28(-2.0),        Q28(+1.0),        Q28(+1.9688775),  Q28(-0.9703966)
};

static int32_t use_exp_float(float_s32_t fl, exponent_t exp)
{
    exponent_t exp_diff = fl.exp - exp;

    if (exp_diff > 0) {
        return fl.mant << exp_diff;
    } else if (exp_diff < 0) {
        return fl.mant >> -exp_diff;
    }

    return fl.mant;
}

void test_hpf(){
    //unsigned seed = 279457;
    int32_t DWORD_ALIGNED filtState[8] = {0};
    int32_t DWORD_ALIGNED samples_orig[len] = {0};
    int32_t DWORD_ALIGNED samples_filt_dsp[len] = {0};
    int32_t DWORD_ALIGNED samples_filt_xs3[len] = {0};
    int32_t DWORD_ALIGNED abs_orig_int[len / 2] = {0};
    int32_t DWORD_ALIGNED abs_filt_int_dsp[len / 2] = {0};
    int32_t DWORD_ALIGNED abs_filt_int_xs3[len / 2] = {0};
    bfp_s32_t orig, filt_dsp, filt_xs3;
    bfp_s32_t orig_abs, filt_abs_dsp, filt_abs_xs3;
    samples_orig[5] = 0x00ffffff;
    xs3_biquad_filter_s32_t filter = {0};
    filter.biquad_count = 2;

    int i = 0, j = 0;
    for(int v = 0; v < 10; v++){
        if(i == 5){i = 0; j++;}
        filter.coef[i][j] = stage_c_filt_coeffs[v];
        i++;
    }

    for(unsigned i = 0; i < len; i++) {
        //samples_orig[i] = pseudo_rand_int(&seed, 0, 0x0000ffff);
        //printf("%ld  ", samples[i]);
        samples_filt_dsp[i] = dsp_filters_biquads(samples_orig[i], stage_c_filt_coeffs, filtState, 2, 28);
        //printf("%ld  ||", samples_orig[i]);
        samples_filt_xs3[i] = xs3_filter_biquad_s32(&filter, samples_orig[i]);
    }
    //printf("\n\n");
    
    bfp_s32_init(&orig, samples_orig, -28, len, 1);
    bfp_s32_init(&filt_dsp, samples_filt_dsp, -28, len, 1);
    bfp_s32_init(&filt_xs3, samples_filt_xs3, -28, len, 1);
    bfp_s32_init(&orig_abs, abs_orig_int, -28, len / 2, 0);
    bfp_s32_init(&filt_abs_dsp, abs_filt_int_dsp, -28, len / 2, 0);
    bfp_s32_init(&filt_abs_xs3, abs_filt_int_xs3, -28, len / 2, 0);
    bfp_complex_s32_t * fd_orig = bfp_fft_forward_mono(&orig);
    //for(int v = 0; v < len; v++)printf("%ld  ||", bfp_orig.data[v]);printf("\n\n");
    bfp_complex_s32_mag(&orig_abs, fd_orig);
    printf("%d %d \n", orig_abs.exp, orig_abs.hr);
    for(int v = 0; v < len / 2; v++)printf("%ld  ||", orig_abs.data[v]);printf("\n\n");

    bfp_complex_s32_t * fd_filt_dsp = bfp_fft_forward_mono(&filt_dsp);
    //for(int v = 0; v < len; v++)printf("%ld  ||", bfp_filt.data[v]);
    bfp_complex_s32_mag(&filt_abs_dsp, fd_filt_dsp);
    printf("%d %d \n", filt_abs_dsp.exp, filt_abs_dsp.hr);
    for(int v = 0; v < len / 2; v++)printf("%ld  ||", filt_abs_dsp.data[v]);printf("\n\n");

    bfp_complex_s32_t * fd_filt_xs3 = bfp_fft_forward_mono(&filt_xs3);
    //for(int v = 0; v < len; v++)printf("%ld  ||", bfp_filt.data[v]);
    bfp_complex_s32_mag(&filt_abs_xs3, fd_filt_xs3);
    printf("%d %d \n", filt_abs_xs3.exp, filt_abs_xs3.hr);
    for(int v = 0; v < len / 2; v++)printf("%ld  ||", filt_abs_xs3.data[v]);printf("\n\n");
    TEST_ASSERT(1);
}