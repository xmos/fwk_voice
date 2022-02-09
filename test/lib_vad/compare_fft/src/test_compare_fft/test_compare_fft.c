// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <bfp_math.h>
#include <math.h>

#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>
#include <dsp.h>

#define EXP  -31
#define lenght 256

TEST_GROUP_RUNNER(test_compare_fft){
    RUN_TEST_CASE(test_compare_fft, case0);
}

TEST_GROUP(test_compare_fft);
TEST_SETUP(test_compare_fft) { fflush(stdout); }
TEST_TEAR_DOWN(test_compare_fft) {}

int dsp_fft(dsp_complex_t * input){

    int hr = dsp_bfp_cls(input, lenght);
    int rel_exp = hr - 1;
    dsp_bfp_shl(input, lenght, rel_exp);

    // First compute frequency domain: input e [-2^31..2^31], output div by N
    dsp_fft_bit_reverse(input, lenght);         
    dsp_fft_forward(input, lenght, dsp_sine_256);
    dsp_fft_split_spectrum(input, lenght);

    return rel_exp;
}

int xs3_math_fft(int32_t * curr){

    complex_s32_t* curr_fd = (complex_s32_t*)curr;

    exponent_t x_exp = -31;
    headroom_t hr = xs3_vect_s32_headroom(curr, lenght);

    right_shift_t x_shr = 2 - hr;
    xs3_vect_s32_shl(curr, curr, lenght, -x_shr);
    hr += x_shr; x_exp += x_shr;

    xs3_fft_index_bit_reversal(curr_fd, lenght/2);
    xs3_fft_dit_forward(curr_fd, lenght / 2, &hr, &x_exp);
    xs3_fft_mono_adjust(curr_fd, lenght, 0);

    return x_exp;
}

TEST(test_compare_fft, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();

    dsp_complex_t DWORD_ALIGNED orig_dsp[lenght];
    int32_t DWORD_ALIGNED orig_xs3[lenght] = {0};
    int32_t * orig_dsp_int;
    float_s32_t freq_dsp;
    float_s32_t freq_xs3;
    double dsp_db;
    double xs3_db;

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < lenght; v++){
            //orig_dsp[v].re = pseudo_rand_int(&seed, 0, INT_MAX);
            orig_dsp[v].re = pseudo_rand_int(&seed, 0, 0xffff);
            //orig_dsp[v].re = 0xffff;
            orig_dsp[v].im = 0;
            orig_xs3[v] = orig_dsp[v].re;
        }


        int rel_dsp_exp = dsp_fft(orig_dsp);
        orig_dsp_int = &orig_dsp[0];
        freq_dsp.exp = -31 - rel_dsp_exp / 2;

        int xs3_exp = xs3_math_fft(orig_xs3);
        freq_xs3.exp = xs3_exp;

        double thresh = ldexp(1, -20);

        for(int v = 0; v < lenght; v++){
            freq_dsp.mant = orig_dsp_int[v];
            freq_xs3.mant = orig_xs3[v];
            dsp_db = float_s32_to_double(freq_dsp);
            xs3_db = float_s32_to_double(freq_xs3);
            double d = fabs(dsp_db - xs3_db);
            TEST_ASSERT(d < thresh);
        }
    }
}