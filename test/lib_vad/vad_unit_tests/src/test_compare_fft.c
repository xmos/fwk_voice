// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_mel.h"
#include "vad_mel_scale.h"
#include "vad_parameters.h"
#include "bfp_math.h"
#include "dsp.h"
#include "xs3_math.h" // for headroom_t
#include "vad_unit_tests.h"

//The implementation held in vad.xc
int dsp_fft(dsp_complex_t * input, int nq){

    int hr = dsp_bfp_cls(input, VAD_PROC_FRAME_LENGTH);
    int rel_exp = hr - 1;
    dsp_bfp_shl(input, VAD_PROC_FRAME_LENGTH, rel_exp);

    // First compute frequency domain: input e [-2^31..2^31], output div by N
    dsp_fft_bit_reverse(input, VAD_PROC_FRAME_LENGTH);         
    dsp_fft_forward(input, VAD_PROC_FRAME_LENGTH, dsp_sine_512);
    if(!nq) dsp_fft_split_spectrum(input, VAD_PROC_FRAME_LENGTH);

    return rel_exp;
}


//Implementation in vad.c
extern headroom_t vad_xs3_math_fft(int32_t * curr, int nq);

int iabs(int a, int b){
    return a > b ? a-b : b-a;
}

void test_compare_fft_mfcc(){
    unsigned seed = 6031759;

    dsp_complex_t DWORD_ALIGNED orig_dsp[VAD_PROC_FRAME_LENGTH + 2];
    int32_t DWORD_ALIGNED orig_xs3[VAD_PROC_FRAME_LENGTH + 2] = {0};
    int32_t DWORD_ALIGNED orig_bfp[VAD_PROC_FRAME_LENGTH + 2] = {0};
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){ //100 gets us to very near full scale

        if((i % 12 ) == 0) max = max<<3;

        for(int v = 0; v < VAD_PROC_FRAME_LENGTH; v++){
            orig_dsp[v].re = pseudo_rand_int(&seed, -max, max);
            orig_dsp[v].im = 0;
            orig_xs3[v] = orig_dsp[v].re;
            orig_bfp[v] = orig_xs3[v];
        }
        // printf("T: %ld\n", orig_dsp[0].re);

        headroom_t dsp_hr = dsp_fft(orig_dsp, 1);
        complex_s32_t * dsp_fd = (complex_s32_t *)orig_dsp;


        headroom_t xs3_hr = vad_xs3_math_fft(orig_xs3, 1);
        complex_s32_t * xs3_fd = (complex_s32_t *)orig_xs3;


        bfp_s32_t bfp;
        bfp_s32_init(&bfp, orig_bfp, -31, VAD_PROC_FRAME_LENGTH, 1);
        bfp_complex_s32_t * bfp_fd = bfp_fft_forward_mono(&bfp);
        bfp_fft_unpack_mono(bfp_fd);


        int max_diff = 4;
        for(int v = 0; v < VAD_PROC_FRAME_LENGTH/2; v++){
            // printf("ex ref: %d xs3: %d bfp: %d\n", 9-rel_dsp_exp, xs3_exp+31, bfp_fd->exp+31);
            // printf("RE ref: %ld xs3: %ld bfp: %ld\n", dsp_fd[v].re, xs3_fd[v].re, bfp_fd->data[v].re);
            // printf("IM ref: %ld xs3: %ld bfp: %ld\n", dsp_fd[v].im, xs3_fd[v].im, bfp_fd->data[v].im);

            //Note we have not exponent adjusted the BFP version so removed from test. We don't use it anyway

            if((dsp_hr != xs3_hr) /*|| (dsp_hr != bfp_fd->exp+31)*/){
                printf("FAIL - ex ref: %d xs3: %d bfp: %d\n", dsp_hr, xs3_hr, bfp_fd->exp+31);
                TEST_ASSERT(0);
            }


            if((iabs(dsp_fd[v].re, xs3_fd[v].re) > max_diff) /*|| (iabs(dsp_fd[v].re, bfp_fd->data[v].re) > max_diff)*/){
                printf("FAIL - RE ref: %ld xs3: %ld bfp: %ld\n", dsp_fd[v].re, xs3_fd[v].re, bfp_fd->data[v].re);
                TEST_ASSERT(0);
            }

            if((iabs(dsp_fd[v].im, xs3_fd[v].im) > max_diff) /*|| (iabs(dsp_fd[v].im, bfp_fd->data[v].im) > max_diff)*/){
                printf("FAIL - IM ref: %ld xs3: %ld bfp: %ld\n", dsp_fd[v].im, xs3_fd[v].im, bfp_fd->data[v].im);
                TEST_ASSERT(0);
            }
        }
    }
}