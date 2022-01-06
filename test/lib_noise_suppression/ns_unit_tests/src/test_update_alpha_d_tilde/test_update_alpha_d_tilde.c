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

#include <suppression.h>
#include <suppression_testing.h>
#include <unity.h>

#include "unity_fixture.h"
#include "../../../../shared/pseudo_rand/pseudo_rand.h"
#include "../../../../shared/testing/testing.h"

#define EXP  -31

TEST_GROUP_RUNNER(ns_update_alpha_d_tilde){
    RUN_TEST_CASE(ns_update_alpha_d_tilde, case0);
}

TEST_GROUP(ns_update_alpha_d_tilde);
TEST_SETUP(ns_update_alpha_d_tilde) { fflush(stdout); }
TEST_TEAR_DOWN(ns_update_alpha_d_tilde) {}

TEST(ns_update_alpha_d_tilde, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [SUP_PROC_FRAME_BINS];
    double actual;

    double alpha_d;
    float_s32_t alpha_d_fl;

    int32_t p_int [SUP_PROC_FRAME_BINS];
    float_s32_t p_fl;
    double p_db;

    for(int i = 0; i < 100; i++){

        sup_state_t state;
        sup_init_state(&state);

        alpha_d_fl.mant = pseudo_rand_int(&seed, 0, INT_MAX);
        alpha_d_fl.exp = EXP;
        sup_set_noise_alpha_d(&state, alpha_d_fl);
        alpha_d = float_s32_to_double(alpha_d_fl);

        for (int v = 0; v < SUP_PROC_FRAME_BINS; v++){

            p_int[v] = pseudo_rand_int(&seed, 0xffffffff, 0x7fffffff);
            p_fl.mant = p_int[v];
            p_fl.exp = EXP;
            p_db = float_s32_to_double(p_fl);

            expected[v] =  alpha_d + ((1.0 - alpha_d) * p_db);
        }

        state.p.data = &p_int[0];
        bfp_s32_headroom(&state.p);

        ns_update_alpha_d_tilde(&state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = state.alpha_d_tilde.data[v];
            act_fl.exp = state.alpha_d_tilde.exp;
            actual = float_s32_to_double(act_fl);

            double t = fabs(expected[v] - actual);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/(expected[id] + ldexp(1, -40)));
        double thresh = ldexp(1, -24);
        TEST_ASSERT(rel_error < thresh);
    }
}