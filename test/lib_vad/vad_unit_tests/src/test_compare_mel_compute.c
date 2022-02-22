// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_mel.h"
#include "vad_mel_scale.h"
#include "vad_parameters.h"
#include "bfp_math.h"
#include "dsp.h"
#include <string.h>

//Note this test assumes that test_compare_fft has worked as we use vad_xs3_math_fft to generate vectors

extern headroom_t vad_xs3_math_fft(int32_t * curr, int nq);

extern void vad_mel_compute_new(int32_t melValues[], uint32_t M,
                                complex_s32_t pts[], uint32_t N,
                                const uint32_t melTable[],
                                int32_t extra_shift) ;

extern void vad_mel_compute(int32_t melValues[], uint32_t M,
                                complex_s32_t pts[], uint32_t N,
                                const uint32_t melTable[],
                                int32_t extra_shift) ;



// extern int log_exponent(uint32_t h, uint32_t l, uint32_t logN);
// extern int log_exponent_new(uint32_t h, uint32_t l, uint32_t logN);

// void do_log_test(void){
//     for(int h=0; h<1000;h++){
//         for(int l=0; l<1000; l++){
//             int logN = -28;
//             int old = log_exponent(h, l, logN);
//             int new = log_exponent_new(h, l, logN);
//             if(new != old){
//                 printf("log_test old: %d new: %d\n", old, new);
//                 TEST_ASSERT(0);
//             }
//         }
//     }
// }


int iabs(int a, int b){
    return a > b ? a-b : b-a;
}

int do_test(complex_s32_t in_vector[VAD_PROC_FRAME_LENGTH + 2], headroom_t headroom){

    int test_passed = 1;

    int32_t old_mel[VAD_N_MEL_SCALE + 1 + 1] = {0}; //+1 because there is a bug in old_mel where it shoots off end of array
    int32_t new_mel[VAD_N_MEL_SCALE + 1] = {0};

    complex_s32_t DWORD_ALIGNED in_vector_copy[VAD_PROC_FRAME_LENGTH + 2];
    memcpy(in_vector_copy, in_vector, sizeof(in_vector_copy));

    // printf("HERE1 %d %d\n", in_vector[0].re, in_vector[0].im);
    // vad_mel_compute(new_mel, VAD_N_MEL_SCALE + 1, in_vector, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
    vad_mel_compute_new(new_mel, VAD_N_MEL_SCALE + 1, in_vector_copy, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
    // printf("HERE2 %d %d\n", in_vector[0].re, in_vector[0].im);
    vad_mel_compute(old_mel, VAD_N_MEL_SCALE + 1, in_vector, VAD_PROC_FRAME_BINS + 1, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
    // printf("HERE3 %d %d\n", in_vector[0].re, in_vector[0].im);

    int max_diff = 100;
    for(int v = 0; v < VAD_N_MEL_SCALE + 1; v++){
        if(iabs(old_mel[v], new_mel[v]) > max_diff){
            printf("FAIL mel: %d: old: %ld new: %ld\n", v, old_mel[v], new_mel[v]);
            test_passed = 0;
        }
    }
    return test_passed;
}


void print_array(int8_t * arr8, unsigned len){
    for(int i=0; i<len;i++){
        printf("0x%x, ", arr8[i]);
    }
    printf("\n");
}

void test_compare_mel(){
    unsigned seed = 6031769; //Google it. Geeky stuff from 40 years ago

    complex_s32_t DWORD_ALIGNED in_vector[VAD_PROC_FRAME_LENGTH + 2];
    int32_t max = 0x0000000f;

    int all_tests_passed = 1;

    // do_log_test(); //Needs modification to old lib_vad to make function non static

    for(int i = 0; i < 190; i++){ //gets us tos full scale (zero HR)
        for(int t = 0; t < 2; t++){ //Gen FFTd data then chuck in raw data to get better coverage
            memset(in_vector, 0, sizeof(in_vector));

            headroom_t headroom = 0;

            if(t == 0){
                for(int v = 0; v < VAD_PROC_FRAME_LENGTH; v++){
                    in_vector[v].re = pseudo_rand_int(&seed, -max, max);
                    in_vector[v].im = 0;
                }
                headroom = vad_xs3_math_fft((int32_t *)in_vector, 1);
                // printf("headroom fftdata: %u\n", headroom);

            }else{
                for(int v = 0; v < VAD_PROC_FRAME_LENGTH; v++){

                    in_vector[v].re = pseudo_rand_int(&seed, -max, max);
                    in_vector[v].im = pseudo_rand_int(&seed, -max, max);
                }

                headroom = xs3_vect_s32_headroom((int32_t*)in_vector, VAD_PROC_FRAME_LENGTH * 2);
                // printf("headroom rawdata: %u\n", headroom);

            }
            int test_passed = do_test(in_vector, headroom);
            // printf("i: %d t: %d pass: %d\n", i, t, test_passed);
            all_tests_passed = all_tests_passed && test_passed;
        }
        if((i % 7 ) == 0) max = max<<1;


    }
    TEST_ASSERT(all_tests_passed);
}