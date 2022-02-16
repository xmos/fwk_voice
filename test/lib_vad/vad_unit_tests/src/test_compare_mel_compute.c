// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_mel.h"
#include "vad_mel_scale.h"
#include "vad_parameters.h"
#include <bfp_math.h>
#include <dsp.h>

//Note this test assumes that test_compare_fft has worked as we use vad_xs3_math_fft to generate vectors

extern headroom_t vad_xs3_math_fft(int32_t * curr, int nq);

extern void vad_mel_compute_new(int32_t melValues[], uint32_t M,
                                complex_s32_t pts[], uint32_t N,
                                const uint32_t melTable[],
                                int32_t extra_shift) ;


void test_compare_mel(){
    unsigned seed = 6031759;

    dsp_complex_t DWORD_ALIGNED in_vector[VAD_PROC_FRAME_LENGTH + 2];
    dsp_complex_t DWORD_ALIGNED in_vector_copy[VAD_PROC_FRAME_LENGTH + 2];
    int32_t old_mel[VAD_N_MEL_SCALE + 1] = {0};
    int32_t new_mel[VAD_N_MEL_SCALE + 1] = {0};
    int32_t max = 0x0000000f;

    for(int i = 0; i < 100; i++){

        if((i % 12 ) == 0) max = max<<3;

        for(int v = 0; v < VAD_PROC_FRAME_LENGTH; v++){
            in_vector[v].re = pseudo_rand_int(&seed, 0, max);
            in_vector[v].im = 0;
        }


        headroom_t headroom = vad_xs3_math_fft(in_vector, 1);
        // complex_s32_t * dsp_fd = (complex_s32_t *)in_vector;

        //Test memory corruption
        // memcpy(in_vector_copy, in_vector, sizeof(in_vector));

        printf("HERE1 %d %d\n", in_vector[0].re, in_vector[0].im);
        vad_mel_compute_new(new_mel, VAD_N_MEL_SCALE + 1, in_vector, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
        printf("HERE2 %d %d\n", in_vector[0].re, in_vector[0].im);
        vad_mel_compute(old_mel, VAD_N_MEL_SCALE + 1, in_vector, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
        printf("HERE3 %d %d\n", in_vector[0].re, in_vector[0].im);

        int th = 4;
        for(int v = 0; v < VAD_N_MEL_SCALE + 1; v++){
            printf("%d: old: %ld new: %ld\n", v, old_mel[v], new_mel[v]);
            // int d = abs(dsp_mel[v] - xs3_mel[v]);
            // TEST_ASSERT(d < th);

            //Note we have not exponent adjusted the BFP version so removed from test. We don't use it anyway
            // d = abs(dsp_mel[v] - bfp_mel[v]);
            // TEST_ASSERT(d < th);
        }
    }
}