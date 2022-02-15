// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_mel.h"
#include "vad_mel_scale.h"
#include "vad_parameters.h"
#include <bfp_math.h>
#include <dsp.h>

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
extern int vad_xs3_math_fft(int32_t * curr, int nq);



void test_compare_fft_mfcc(){
    unsigned seed = 6031759;

    dsp_complex_t DWORD_ALIGNED orig_dsp[VAD_PROC_FRAME_LENGTH + 2];
    int32_t DWORD_ALIGNED orig_xs3[VAD_PROC_FRAME_LENGTH + 2] = {0};
    int32_t DWORD_ALIGNED orig_bfp[VAD_PROC_FRAME_LENGTH + 2] = {0};
    int32_t dsp_mel[VAD_N_MEL_SCALE + 1] = {0};
    int32_t xs3_mel[VAD_N_MEL_SCALE + 1] = {0};
    int32_t bfp_mel[VAD_N_MEL_SCALE + 1] = {0};
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){

        if((i % 12 ) == 0) max = max<<3;

        for(int v = 0; v < VAD_PROC_FRAME_LENGTH; v++){
            orig_dsp[v].re = pseudo_rand_int(&seed, 0, max);
            orig_dsp[v].im = 0;
            orig_xs3[v] = orig_dsp[v].re;
            orig_bfp[v] = orig_xs3[v];
        }

        int rel_dsp_exp = dsp_fft(orig_dsp, 1);
        complex_s32_t * dsp_fd = (complex_s32_t *)orig_dsp;

        vad_mel_compute(dsp_mel, VAD_N_MEL_SCALE + 1, dsp_fd, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2 * (VAD_LOG_WINDOW_LENGTH - rel_dsp_exp));

        int xs3_exp = vad_xs3_math_fft(orig_xs3, 1);
        complex_s32_t * xs3_fd = (complex_s32_t *)orig_xs3;

        vad_mel_compute(xs3_mel, VAD_N_MEL_SCALE + 1, xs3_fd, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, - (-31 - xs3_exp) * 2);

        bfp_s32_t bfp;
        bfp_s32_init(&bfp, orig_bfp, -31, VAD_PROC_FRAME_LENGTH, 1);
        bfp_complex_s32_t * bfp_fd = bfp_fft_forward_mono(&bfp);
        bfp_fft_unpack_mono(bfp_fd);

        vad_mel_compute(bfp_mel, VAD_N_MEL_SCALE + 1, bfp_fd->data, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, - (-31 - bfp_fd->exp) * 2);

        int th = 4;
        for(int v = 0; v < VAD_N_MEL_SCALE + 1; v++){
            int d = abs(dsp_mel[v] - xs3_mel[v]);
            TEST_ASSERT(d < th);
            d = abs(dsp_mel[v] - bfp_mel[v]);
            TEST_ASSERT(d < th);
        }
    }
}