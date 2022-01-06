// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <xs3_math.h>
#include <bfp_init.h>
#include <bfp_s32.h>
#include <math.h>

#include <suppression.h>
#include <suppression_testing.h>
#include <unity.h>

#include "unity_fixture.h"
#include "../../../../shared/pseudo_rand/pseudo_rand.h"
#include "../../../../shared/testing/testing.h"

#define EXP  -31

TEST_GROUP_RUNNER(ns_minimum){
    RUN_TEST_CASE(ns_minimum, case0);
}

TEST_GROUP(ns_minimum);
TEST_SETUP(ns_minimum) { fflush(stdout); }
TEST_TEAR_DOWN(ns_minimum) {}

TEST(ns_minimum, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    int32_t S_int [SUP_PROC_FRAME_BINS];
    float_s32_t S_fl;
    double S_db [SUP_PROC_FRAME_BINS];

    double expected [SUP_PROC_FRAME_BINS];
    double actual [SUP_PROC_FRAME_BINS];
    int32_t S_min_int [SUP_PROC_FRAME_BINS];

    int32_t S_tmp_int [SUP_PROC_FRAME_BINS];
    float_s32_t S_tmp_fl;
    double S_tmp_db [SUP_PROC_FRAME_BINS];

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            S_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            S_fl.mant = S_int[v];
            S_fl.exp = INT_EXP;
            S_db[v] = float_s32_to_double(S_fl);

            S_tmp_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            S_tmp_fl.mant = S_tmp_int[v];
            S_tmp_fl.exp = INT_EXP + 2;
            S_tmp_db[v] = float_s32_to_double(S_tmp_fl);

            if (S_tmp_db[v] < S_db[v]){expected[v] = S_tmp_db[v];}
            else{expected[v] = S_db[v];}
        }

        bfp_s32_t S_bfp, S_tmp_bfp, S_min_bfp;
        bfp_s32_init(&S_bfp, S_int, INT_EXP, SUP_PROC_FRAME_BINS, 1);
        bfp_s32_init(&S_tmp_bfp, S_tmp_int, INT_EXP + 2, SUP_PROC_FRAME_BINS, 1);
        bfp_s32_init(&S_min_bfp, S_min_int, INT_EXP, SUP_PROC_FRAME_BINS, 0);

        ns_adjust_exp(&S_min_bfp, &S_tmp_bfp, &S_bfp);
        ns_minimum(&S_min_bfp, &S_tmp_bfp, &S_bfp);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = S_min_bfp.data[v];
            act_fl.exp = S_min_bfp.exp;
            actual[v] = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual[v]);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/expected[id]);
        double thresh = ldexp(1, -30);
        TEST_ASSERT(rel_error < thresh);
    }
}