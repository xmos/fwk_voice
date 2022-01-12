// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <xs3_math.h>
#include <bfp_init.h>

#include <suppression.h>
#include <suppression_testing.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31

TEST_GROUP_RUNNER(ns_update_S){
    RUN_TEST_CASE(ns_update_S, case0);
}

TEST_GROUP(ns_update_S);
TEST_SETUP(ns_update_S) { fflush(stdout); }
TEST_TEAR_DOWN(ns_update_S) {}

TEST(ns_update_S, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();

    double alpha_s;
    float_s32_t alpha_s_fl;

    double expected [SUP_PROC_FRAME_BINS];
    double actual;

    int32_t abs_Y_int [SUP_PROC_FRAME_BINS];
    float_s32_t abs_Y_fl;
    double abs_Y_db;

    for(int i = 0; i < 100; i++){

        sup_state_t state;
        sup_init_state(&state);
        alpha_s_fl.mant = pseudo_rand_int(&seed, 0, INT_MAX);
        alpha_s_fl.exp = EXP;
        state.alpha_s = alpha_s_fl;
        alpha_s = float_s32_to_double(alpha_s_fl);

        for (int v = 0; v < SUP_PROC_FRAME_BINS; v++){

            abs_Y_int[v] = pseudo_rand_int(&seed, 0x70000000, 0x7fffffff);
            abs_Y_fl.mant = abs_Y_int[v];
            abs_Y_fl.exp = EXP;
            abs_Y_db = float_s32_to_double(abs_Y_fl);
            expected[v] = 1.0;

            expected[v] = (expected[v] * alpha_s) + ((1.0 - alpha_s) * (abs_Y_db * abs_Y_db));
        }

        bfp_s32_t abs_Y_bfp;
        bfp_s32_init(&abs_Y_bfp, abs_Y_int, EXP, SUP_PROC_FRAME_BINS, 1);

        ns_update_S(&state, &abs_Y_bfp);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = state.S.data[v];
            act_fl.exp = state.S.exp;
            actual = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/(expected[id] + ldexp(1, -40)));
        double thresh = ldexp(1, -28);
        TEST_ASSERT(rel_error < thresh);
    }
}