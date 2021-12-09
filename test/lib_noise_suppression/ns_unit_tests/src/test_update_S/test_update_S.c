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
#include "../../../../shared/pseudo_rand/pseudo_rand.h"
#include "../../../../shared/testing/testing.h"

#define EXP  -31

TEST_GROUP_RUNNER(update_S){
    RUN_TEST_CASE(update_S, case0);
}

TEST_GROUP(update_S);
TEST_SETUP(update_S) { fflush(stdout); }
TEST_TEAR_DOWN(update_S) {}

TEST(update_S, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [SUP_PROC_FRAME_BINS];
    double actual [SUP_PROC_FRAME_BINS];
    double abs_Y_db [SUP_PROC_FRAME_BINS];
    double alpha_s = 0.8;
    int32_t abs_Y_int [SUP_PROC_FRAME_BINS];
    float_s32_t abs_Y_fl [SUP_PROC_FRAME_BINS];

    for(int i = 0; i < 100; i++){

        suppression_state_t state;
        sup_init_state(&state);
        sup_set_noise_alpha_s(&state, double_to_float_s32(alpha_s));

        for (int v = 0; v < SUP_PROC_FRAME_BINS; v++){

            abs_Y_int[v] = pseudo_rand_int(&seed, 0xafffffff, 0x7fffffff);
            abs_Y_fl[v].mant = abs_Y_int[v];
            abs_Y_fl[v].exp = EXP;
            abs_Y_db[v] = float_s32_to_double(abs_Y_fl[v]);
            expected[v] = 1.0;

            expected[v] = (expected[v] * alpha_s) + ((1.0 - alpha_s) * (abs_Y_db[v] * abs_Y_db[v]));
        }

        bfp_s32_t abs_Y_bfp;
        bfp_s32_init(&abs_Y_bfp, abs_Y_int, EXP, SUP_PROC_FRAME_BINS, 0);

        ns_update_S(&abs_Y_bfp, &state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = state.S.data[v];
            act_fl.exp = state.S.exp;
            actual[v] = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual[v]);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/expected[id]);
        double thresh = ldexp(1, -28);
        TEST_ASSERT(rel_error < thresh);
    }
}