// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "xmath/xmath.h"
#include <math.h>

#include <ns_api.h>
#include <ns_priv.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31


TEST_GROUP_RUNNER(ns_priv_rescale_vector){
    RUN_TEST_CASE(ns_priv_rescale_vector, case0);
}

TEST_GROUP(ns_priv_rescale_vector);
TEST_SETUP(ns_priv_rescale_vector) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_rescale_vector) {}

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

TEST(ns_priv_rescale_vector, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();

    int32_t abs_orig_int[NS_PROC_FRAME_BINS];
    int32_t abs_ns_int[NS_PROC_FRAME_BINS];
    complex_s32_t Y_int[NS_PROC_FRAME_BINS];
    float_s32_t t;
    double abs_ratio;
    double expected[NS_PROC_FRAME_BINS * 2];
    float_s32_t ex_re_fl, ex_im_fl;
    double orig, new;

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            abs_orig_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            abs_ns_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            while(abs_orig_int[v] < abs_ns_int[v])
                abs_ns_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            t.mant = abs_orig_int[v];
            t.exp = EXP;
            orig = float_s32_to_double(t);
            t.mant = abs_ns_int[v];
            t.exp = EXP;
            new = float_s32_to_double(t);
            abs_ratio = new / orig;

            Y_int[v].re = pseudo_rand_int(&seed, INT_MIN, INT_MAX)>>1;
            t.mant = Y_int[v].re;
            t.exp = EXP;
            expected[2 * v] = float_s32_to_double(t) * abs_ratio;

            Y_int[v].im = pseudo_rand_int(&seed, INT_MIN, INT_MAX)>>1;
            t.mant = Y_int[v].im;
            t.exp = EXP;
            expected[(2 * v) + 1] = float_s32_to_double(t) * abs_ratio;
        }

        bfp_s32_t abs_orig, abs_ns;
        bfp_complex_s32_t Y;
        bfp_s32_init(&abs_orig, abs_orig_int, EXP, NS_PROC_FRAME_BINS, 1);
        bfp_s32_init(&abs_ns, abs_ns_int, EXP, NS_PROC_FRAME_BINS, 1);
        bfp_complex_s32_init(&Y, Y_int, EXP, NS_PROC_FRAME_BINS, 1);

        ns_priv_rescale_vector(&Y, &abs_ns, &abs_orig);

        int32_t abs_diff = 0;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            int32_t d_r, d_i, re_int, im_int;

            ex_re_fl = f64_to_float_s32(expected[2 * v]);
            ex_im_fl = f64_to_float_s32(expected[(2 * v) + 1]);

            re_int = use_exp_float(ex_re_fl, Y.exp);
            im_int = use_exp_float(ex_im_fl, Y.exp);

            d_r = abs(re_int - Y.data[v].re);
            d_i = abs(im_int - Y.data[v].im);

            if(d_i > d_r)
                d_r = d_i;
            if(d_r > abs_diff)
                abs_diff = d_r;
        }
        TEST_ASSERT(abs_diff <= 8);
    }
}