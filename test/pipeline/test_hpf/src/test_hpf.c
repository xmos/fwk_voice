// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf_test.h"

#define len 512

int32_t final_coef[10] = {
    1020035168, -2040070348, 1020035168, 2070720224, -998576072,
    1073741824, -2147483648, 1073741824, 2114066120, -1041955416
};

void test_hpf(){
    int32_t DWORD_ALIGNED samples_orig[len + 2] = {0};
    int32_t DWORD_ALIGNED samples_filt[len + 2] = {0};
    int32_t DWORD_ALIGNED abs_orig_int[len / 2 + 1] = {0};
    int32_t DWORD_ALIGNED abs_filt_int[len / 2 + 1] = {0};
    bfp_s32_t orig, filt;
    bfp_s32_t orig_abs, filt_abs;
    float_s32_t orig_fl, filt_fl;
    double orig_db, filt_db;
    xs3_biquad_filter_s32_t filter = {0};
    filter.biquad_count = 2;

    int i = 0, j = 0;
    for(int v = 0; v < 10; v++){
        if(i == 5){i = 0; j++;}
        filter.coef[i][j] = final_coef[v];
        i++;
    }

    samples_orig[5] = 0x3fffffff;
    for(unsigned i = 0; i < len; i++) {
        samples_orig[i] >>= 1;
        samples_filt[i] = xs3_filter_biquad_s32(&filter, samples_orig[i]);
    }

    bfp_s32_init(&orig, samples_orig, -31, len, 1);
    bfp_s32_init(&filt, samples_filt, -31, len, 1);
    bfp_s32_init(&orig_abs, abs_orig_int, -31, len / 2 + 1, 0);
    bfp_s32_init(&filt_abs, abs_filt_int, -31, len / 2 + 1, 0);

    bfp_complex_s32_t * fd_orig = bfp_fft_forward_mono(&orig);
    bfp_fft_unpack_mono(fd_orig);

    bfp_complex_s32_mag(&orig_abs, fd_orig);

    bfp_complex_s32_t * fd_filt = bfp_fft_forward_mono(&filt);
    bfp_fft_unpack_mono(fd_filt);

    bfp_complex_s32_mag(&filt_abs, fd_filt);

    orig_fl.exp = orig_abs.exp; filt_fl.exp = filt_abs.exp;
    for(int v = 0; v < len / 2 + 1; v++){
        orig_fl.mant = orig_abs.data[v];
        filt_fl.mant = filt_abs.data[v];
        orig_db = float_s32_to_double(orig_fl);
        filt_db = float_s32_to_double(filt_fl);
        TEST_ASSERT((orig_db + 0.001) > filt_db);
    }
}