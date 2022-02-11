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
#include <vad_mel.h>

#define EXP  -24
#define lenght 512

TEST_GROUP_RUNNER(test_compare_fft_mfcc){
    RUN_TEST_CASE(test_compare_fft_mfcc, no_nyquist);
    RUN_TEST_CASE(test_compare_fft_mfcc, nyquist);
}

const uint32_t vad_mel[257] = {
    0, 8388608, 16777216, 11184810, 5592405, 0, 8388608, 16777216,
    12582912, 8388608, 4194304, 0, 5592405, 11184810, 16777216, 12582912,
    8388608, 4194304, 0, 3355443, 6710886, 10066329, 13421772, 16777216,
    12582912, 8388608, 4194304, 0, 2796202, 5592405, 8388608, 11184810,
    13981013, 16777216, 13981013, 11184810, 8388608, 5592405, 2796202, 0,
    2796202, 5592405, 8388608, 11184810, 13981013, 16777216, 14380470, 11983725,
    9586980, 7190235, 4793490, 2396745, 0, 2097152, 4194304, 6291456,
    8388608, 10485760, 12582912, 14680064, 16777216, 14913080, 13048945, 11184810,
    9320675, 7456540, 5592405, 3728270, 1864135, 0, 1677721, 3355443,
    5033164, 6710886, 8388608, 10066329, 11744051, 13421772, 15099494, 16777216,
    15252014, 13726813, 12201611, 10676410, 9151208, 7626007, 6100805, 4575604,
    3050402, 1525201, 0, 1398101, 2796202, 4194304, 5592405,6990506,
    8388608, 9786709, 11184810, 12582912, 13981013, 15379114, 16777216, 15486660,
    14196105, 12905550, 11614995, 10324440, 9033885, 7743330, 6452775, 5162220,
    3871665, 2581110, 1290555, 0, 1198372, 2396745, 3595117, 4793490,
    5991862, 7190235, 8388608, 9586980, 10785353, 11983725, 13182098, 14380470,
    15578843, 16777216, 15790320, 14803425, 13816530, 12829635, 11842740, 10855845,
    9868950, 8882055, 7895160, 6908265, 5921370, 4934475, 3947580, 2960685,
    1973790, 986895, 0, 986895, 1973790, 2960685, 3947580, 4934475,
    5921370, 6908265, 7895160, 8882055, 9868950, 10855845, 11842740, 12829635,
    13816530, 14803425, 15790320, 16777216, 15938355, 15099494, 14260633, 13421772,
    12582912, 11744051, 10905190, 10066329, 9227468, 8388608, 7549747, 6710886,
    5872025, 5033164, 4194304, 3355443, 2516582, 1677721, 838860, 0,
    762600, 1525201, 2287802, 3050402, 3813003, 4575604, 5338205, 6100805,
    6863406, 7626007, 8388608, 9151208, 9913809, 10676410, 11439010, 12201611,
    12964212, 13726813, 14489413, 15252014, 16014615, 16777216, 16078165, 15379114,
    14680064, 13981013, 13281962, 12582912, 11883861, 11184810, 10485760, 9786709,
    9087658, 8388608, 7689557, 6990506, 6291456, 5592405, 4893354, 4194304,
    3495253, 2796202, 2097152, 1398101, 699050, 0, 621378, 1242756,
    1864135, 2485513, 3106891, 3728270, 4349648, 4971026, 5592405, 6213783,
    6835162, 7456540, 8077918, 8699297, 9320675, 9942053, 10563432, 11184810,
    11806189, 12427567, 13048945, 13670324, 14291702, 14913080, 15534459, 16155837,
    16777216,
};

TEST_GROUP(test_compare_fft_mfcc);
TEST_SETUP(test_compare_fft_mfcc) { fflush(stdout); }
TEST_TEAR_DOWN(test_compare_fft_mfcc) {}

int dsp_fft_mfcc(dsp_complex_t * input, int nq){

    int hr = dsp_bfp_cls(input, lenght);
    int rel_exp = hr - 1;
    dsp_bfp_shl(input, lenght, rel_exp);

    // First compute frequency domain: input e [-2^31..2^31], output div by N
    dsp_fft_bit_reverse(input, lenght);         
    dsp_fft_forward(input, lenght, dsp_sine_512);
    if(!nq) dsp_fft_split_spectrum(input, lenght);

    return rel_exp;
}

int xs3_math_fft_mfcc(int32_t * curr, int nq){

    complex_s32_t* curr_fd = (complex_s32_t*)curr;

    exponent_t x_exp = -31;
    headroom_t hr = xs3_vect_s32_headroom(curr, lenght);

    right_shift_t x_shr = 2 - hr;
    xs3_vect_s32_shl(curr, curr, lenght, -x_shr);
    hr += x_shr; x_exp += x_shr;

    xs3_fft_index_bit_reversal(curr_fd, lenght / 2);
    xs3_fft_dit_forward(curr_fd, lenght / 2, &hr, &x_exp);
    xs3_fft_mono_adjust(curr_fd, lenght, 0);

    if(nq){
        curr_fd[lenght / 2].re = curr_fd[0].im;
        curr_fd[0].im = 0;
        curr_fd[lenght / 2].im = 0;
    }

    return x_exp;
}

TEST(test_compare_fft_mfcc, no_nyquist){
    unsigned seed = SEED_FROM_FUNC_NAME();

    dsp_complex_t DWORD_ALIGNED orig_dsp[lenght];
    int32_t DWORD_ALIGNED orig_xs3[lenght] = {0};
    int32_t DWORD_ALIGNED orig_bfp[lenght] = {0};
    int32_t dsp_mel[25];
    int32_t xs3_mel[25];
    int32_t bfp_mel[25];
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){

        if((i % 12 ) == 0) max = max<<3;

        for(int v = 0; v < lenght; v++){
            orig_dsp[v].re = pseudo_rand_int(&seed, 0, max);
            orig_dsp[v].im = 0;
            orig_xs3[v] = orig_dsp[v].re;
            orig_bfp[v] = orig_xs3[v];
        }

        int rel_dsp_exp = dsp_fft_mfcc(orig_dsp, 0);
        complex_s32_t * dsp_fd = (complex_s32_t *)orig_dsp;

        vad_mel_compute(dsp_mel, 25, dsp_fd, (lenght / 2), vad_mel, 2 * (9 - rel_dsp_exp));

        int xs3_exp = xs3_math_fft_mfcc(orig_xs3, 0);
        complex_s32_t * xs3_fd = (complex_s32_t *)orig_xs3;

        vad_mel_compute(xs3_mel, 25, xs3_fd, (lenght / 2), vad_mel, - (-31 - xs3_exp) * 2);

        bfp_s32_t bfp;
        bfp_s32_init(&bfp, orig_bfp, -31, lenght, 1);
        bfp_complex_s32_t * bfp_fd = bfp_fft_forward_mono(&bfp);

        vad_mel_compute(bfp_mel, 25, bfp_fd->data, (lenght / 2), vad_mel, - (-31 - bfp_fd->exp) * 2);

        int th = 4;
        for(int v = 0; v < 25; v++){
            int d = abs(dsp_mel[v] - xs3_mel[v]);
            TEST_ASSERT(d < th);
            d = abs(dsp_mel[v] - bfp_mel[v]);
            TEST_ASSERT(d < th);
        }
    }
}

TEST(test_compare_fft_mfcc, nyquist){
    unsigned seed = SEED_FROM_FUNC_NAME();

    dsp_complex_t DWORD_ALIGNED orig_dsp[lenght + 2];
    int32_t DWORD_ALIGNED orig_xs3[lenght + 2] = {0};
    int32_t DWORD_ALIGNED orig_bfp[lenght + 2] = {0};
    int32_t dsp_mel[25] = {0};
    int32_t xs3_mel[25] = {0};
    int32_t bfp_mel[25] = {0};
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){

        if((i % 12 ) == 0) max = max<<3;

        for(int v = 0; v < lenght; v++){
            orig_dsp[v].re = pseudo_rand_int(&seed, 0, max);
            orig_dsp[v].im = 0;
            orig_xs3[v] = orig_dsp[v].re;
            orig_bfp[v] = orig_xs3[v];
        }

        int rel_dsp_exp = dsp_fft_mfcc(orig_dsp, 1);
        complex_s32_t * dsp_fd = (complex_s32_t *)orig_dsp;

        vad_mel_compute(dsp_mel, 25, dsp_fd, (lenght / 2) + 1, vad_mel, 2 * (9 - rel_dsp_exp));

        int xs3_exp = xs3_math_fft_mfcc(orig_xs3, 1);
        complex_s32_t * xs3_fd = (complex_s32_t *)orig_xs3;

        vad_mel_compute(xs3_mel, 25, xs3_fd, (lenght / 2) + 1, vad_mel, - (-31 - xs3_exp) * 2);

        bfp_s32_t bfp;
        bfp_s32_init(&bfp, orig_bfp, -31, lenght, 1);
        bfp_complex_s32_t * bfp_fd = bfp_fft_forward_mono(&bfp);
        bfp_fft_unpack_mono(bfp_fd);

        vad_mel_compute(bfp_mel, 25, bfp_fd->data, (lenght / 2) + 1, vad_mel, - (-31 - bfp_fd->exp) * 2);

        int th = 4;
        for(int v = 0; v < 25; v++){
            int d = abs(dsp_mel[v] - xs3_mel[v]);
            TEST_ASSERT(d < th);
            d = abs(dsp_mel[v] - bfp_mel[v]);
            TEST_ASSERT(d < th);
        }
    }
}
