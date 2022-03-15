// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf_test.h"

#define len 512

void test_hpf_imp_res(){
    int32_t DWORD_ALIGNED samples_orig[len + 2] = {0};
    int32_t DWORD_ALIGNED samples_filt[len + 2] = {0};
    int32_t DWORD_ALIGNED abs_orig_int[len / 2 + 1] = {0};
    int32_t DWORD_ALIGNED abs_filt_int[len / 2 + 1] = {0};
    bfp_s32_t orig, filt;
    bfp_s32_t orig_abs, filt_abs;
    float_s32_t orig_fl, filt_fl;
    double orig_db, filt_db;
    samples_orig[5] = 0x3fffffff;
    samples_filt[5] = 0x3fffffff;

    pre_agc_hpf(samples_filt);

    bfp_s32_init(&orig, samples_orig, NORM_EXP, len, 1);
    bfp_s32_init(&filt, samples_filt, NORM_EXP, len, 1);
    bfp_s32_init(&orig_abs, abs_orig_int, NORM_EXP, len / 2 + 1, 0);
    bfp_s32_init(&filt_abs, abs_filt_int, NORM_EXP, len / 2 + 1, 0);

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
        // test the attenuation of the first bin(16.625 Hz)
        // -19.6 dB is the best I could get with these coefficoents
        if(v == 1){
            TEST_ASSERT((orig_db / filt_db) > 90.0);
        }
        // make sure it converges
        if(v == (len / 2)){
            TEST_ASSERT(fabs(orig_db - filt_db) < 0.006);
        }
        // this test is designed to test the impulse responce only!
        // that's why the test condition is set so the filtered magnitudes
        // have to be smaller as they should only tend to converge 
        // to the flat line (impulse in frequency domain)

        // threshold is high because pre_agc_hpf works on a 240 frame
        // so we'll have an implulse in 240's sample 
        // which will slightly increase all the frequency compnents
        TEST_ASSERT((orig_db + 0.006) > filt_db);
    }
}
