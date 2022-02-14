// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <bfp_math.h>
#include <math.h>

#include <ns_api.h>
#include <ns_test.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31


TEST_GROUP_RUNNER(ns_rescale_vector){
    RUN_TEST_CASE(ns_rescale_vector, case0);
}

TEST_GROUP(ns_rescale_vector);
TEST_SETUP(ns_rescale_vector) { fflush(stdout); }
TEST_TEAR_DOWN(ns_rescale_vector) {}

int32_t use_exp_float(float_s32_t fl, exponent_t exp)
{

    if(fl.exp > exp){
        if(fl.mant > 0){
            return INT_MAX;
        } else {
            return INT_MIN;
        }
    } else {
        return fl.mant >> (exp - fl.exp);
    }
}

void check_saturation(float_s32_t *fl, bfp_complex_s32_t *Y, int v){
    while(use_exp_float(*fl, Y->exp) == INT_MAX){
        fl->mant /= 2; Y->data[v].re /= 2;
    }
}

TEST(ns_rescale_vector, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();

    int32_t abs_orig_int[NS_PROC_FRAME_BINS];
    int32_t abs_ns_int[NS_PROC_FRAME_BINS];
    complex_s32_t Y_int[NS_PROC_FRAME_BINS];
    float_s32_t t, t1;
    int32_t ex_re[NS_PROC_FRAME_BINS], ex_im[NS_PROC_FRAME_BINS];
    double abs_ratio;
    double expected[NS_PROC_FRAME_BINS * 2];
    float_s32_t ex_re_fl, ex_im_fl;

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            abs_orig_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            abs_ns_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);

            t.mant = abs_orig_int[v];
            t.exp = EXP;
            t1.mant = abs_ns_int[v];
            t1.exp = EXP;
            t = float_s32_div(t1, t);
            abs_ratio = float_s32_to_double(t);

            Y_int[v].re = pseudo_rand_int(&seed, 0, INT_MAX);
            t.mant = Y_int[v].re;
            t.exp = EXP;
            expected[2 * v] = float_s32_to_double(t) * abs_ratio;


            Y_int[v].im = pseudo_rand_int(&seed, 0, INT_MAX);
            t1.mant = Y_int[v].im;
            t1.exp = EXP;
            expected[(2 * v) + 1] = float_s32_to_double(t1) * abs_ratio;
        }

        bfp_s32_t abs_orig, abs_ns;
        bfp_complex_s32_t Y;
        bfp_s32_init(&abs_orig, abs_orig_int, EXP, NS_PROC_FRAME_BINS, 1);
        bfp_s32_init(&abs_ns, abs_ns_int, EXP, NS_PROC_FRAME_BINS, 1);
        bfp_complex_s32_init(&Y, Y_int, EXP, NS_PROC_FRAME_BINS, 1);

        ns_rescale_vector(&Y, &abs_ns, &abs_orig);

        int32_t abs_diff = 0;
        int id = 0;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            float_s32_t act_re_fl, act_im_fl;
            int32_t d_r, d_i, re_int, im_int;
            int i;

            ex_re_fl = double_to_float_s32(expected[2 * v]);
            ex_im_fl = double_to_float_s32(expected[(2 * v) + 1]);

            check_saturation(&ex_re_fl, &Y, v);
            check_saturation(&ex_im_fl, &Y, v);

            re_int = use_exp_float(ex_re_fl, Y.exp);
            im_int = use_exp_float(ex_im_fl, Y.exp);
            
            d_r = abs(re_int - Y.data[v].re); i = 2 * v;
            d_i = abs(im_int - Y.data[v].im);

            if(d_i > d_r){d_r = d_i; i++;}
            if(d_r > abs_diff){abs_diff = d_r; id = i;}
        }
        
        TEST_ASSERT(abs_diff <= 4);
    }
}