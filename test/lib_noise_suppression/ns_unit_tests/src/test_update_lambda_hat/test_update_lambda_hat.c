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

TEST_GROUP_RUNNER(ns_update_lambda_hat){
    RUN_TEST_CASE(ns_update_lambda_hat, case0);
}

TEST_GROUP(ns_update_lambda_hat);
TEST_SETUP(ns_update_lambda_hat) { fflush(stdout); }
TEST_TEAR_DOWN(ns_update_lambda_hat) {}

TEST(ns_update_lambda_hat, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [SUP_PROC_FRAME_BINS];
    double actual;

    double abs_Y_db;
    int32_t abs_Y_int [SUP_PROC_FRAME_BINS];
    float_s32_t abs_Y_fl;

    double adt_db;
    int32_t adt_int [SUP_PROC_FRAME_BINS];
    float_s32_t adt_fl;

    for(int i = 0; i < 100; i++){

        suppression_state_t state;
        sup_init_state(&state);

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            abs_Y_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            abs_Y_fl.mant = abs_Y_int[v];
            abs_Y_fl.exp = EXP;
            abs_Y_db = float_s32_to_double(abs_Y_fl);
            expected[v] = 0.0;

            adt_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            adt_fl.mant = adt_int[v];
            adt_fl.exp = EXP;
            adt_db = float_s32_to_double(adt_fl);

            expected[v] = expected[v] * adt_db + (1.0 - adt_db) * (abs_Y_db * abs_Y_db); 
        }

        bfp_s32_t abs_Y_bfp;
        bfp_s32_init(&abs_Y_bfp, abs_Y_int, EXP, SUP_PROC_FRAME_BINS, 0);
        state.alpha_d_tilde.data = &adt_int[0];

        ns_update_lambda_hat(&abs_Y_bfp, &state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = state.lambda_hat.data[v];
            act_fl.exp = state.lambda_hat.exp;
            actual = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/(expected[id] + ldexp(1, -40)));
        double thresh = ldexp(1, -26);
        TEST_ASSERT(rel_error < thresh);
    }
}