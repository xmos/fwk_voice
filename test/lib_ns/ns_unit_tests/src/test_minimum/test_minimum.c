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

TEST_GROUP_RUNNER(ns_priv_minimum){
    RUN_TEST_CASE(ns_priv_minimum, case0);
}

TEST_GROUP(ns_priv_minimum);
TEST_SETUP(ns_priv_minimum) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_minimum) {}

TEST(ns_priv_minimum, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    int32_t S_int [NS_PROC_FRAME_BINS];
    float_s32_t S_fl;
    double S_db [NS_PROC_FRAME_BINS];

    double expected [NS_PROC_FRAME_BINS];
    double actual [NS_PROC_FRAME_BINS];
    int32_t S_min_int [NS_PROC_FRAME_BINS];

    int32_t S_tmp_int [NS_PROC_FRAME_BINS];
    float_s32_t S_tmp_fl;
    double S_tmp_db [NS_PROC_FRAME_BINS];

    for(int i = 0; i < 100; i++){

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            S_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            S_fl.mant = S_int[v];
            S_fl.exp = EXP;
            S_db[v] = float_s32_to_double(S_fl);

            S_tmp_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            S_tmp_fl.mant = S_tmp_int[v];
            S_tmp_fl.exp = EXP + 2;
            S_tmp_db[v] = float_s32_to_double(S_tmp_fl);

            if (S_tmp_db[v] < S_db[v]){expected[v] = S_tmp_db[v];}
            else{expected[v] = S_db[v];}
        }

        bfp_s32_t S_bfp, S_tmp_bfp, S_min_bfp;
        bfp_s32_init(&S_bfp, S_int, EXP, NS_PROC_FRAME_BINS, 1);
        bfp_s32_init(&S_tmp_bfp, S_tmp_int, EXP + 2, NS_PROC_FRAME_BINS, 1);
        bfp_s32_init(&S_min_bfp, S_min_int, EXP, NS_PROC_FRAME_BINS, 0);

        bfp_s32_use_exponent(&S_min_bfp, S_bfp.exp);
        bfp_s32_use_exponent(&S_tmp_bfp, S_bfp.exp);
        ns_priv_minimum(&S_min_bfp, &S_tmp_bfp, &S_bfp);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
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