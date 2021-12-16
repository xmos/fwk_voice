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

TEST_GROUP_RUNNER(ns_update_p){
    RUN_TEST_CASE(ns_update_p, case0);
}

TEST_GROUP(ns_update_p);
TEST_SETUP(ns_update_p) { fflush(stdout); }
TEST_TEAR_DOWN(ns_update_p) {}

TEST(ns_update_p, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();

    double alpha_p;
    double delta;

    float_s32_t alpha_p_fl;
    float_s32_t delta_fl;

    int32_t S_int [SUP_PROC_FRAME_BINS];
    int32_t S_min_int [SUP_PROC_FRAME_BINS];
    float_s32_t S_fl;
    float_s32_t S_min_fl;
    double S_db;
    double S_min_db;

    double expected [SUP_PROC_FRAME_BINS];
    double actual;


    for(int i = 0; i < 100; i++){

        suppression_state_t state;
        sup_init_state(&state);
        
        alpha_p_fl.mant = pseudo_rand_int(&seed, 0, INT_MAX);
        alpha_p_fl.exp = EXP;
        sup_set_noise_alpha_p(&state, alpha_p_fl);
        alpha_p = float_s32_to_double(alpha_p_fl);

        delta_fl.mant = pseudo_rand_int(&seed, 0, INT_MAX);
        delta_fl.exp = EXP;
        delta_fl = float_s32_add(delta_fl, float_to_float_s32(1.0));
        sup_set_noise_delta(&state, delta_fl);
        delta = float_s32_to_double(delta_fl);

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            
            S_int[v] = pseudo_rand_int(&seed, 0x1bbbbbbb, 0x7fffffff);
            S_fl.mant = S_int[v];
            S_fl.exp = INT_EXP;
            S_db = float_s32_to_double(S_fl);

            S_min_int[v] = pseudo_rand_int(&seed, 0x11111111, 0x1ddddddd);
            S_min_fl.mant = S_min_int[v];
            S_min_fl.exp = INT_EXP;
            S_min_db = float_s32_to_double(S_min_fl);

            expected[v] = 0.0;

            expected[v] *= alpha_p;

            if((S_db / S_min_db) > delta){
                expected[v] += (1.0 - alpha_p);
            }
        }

        state.S.data = &S_int[0];
        state.S_min.data = &S_min_int[0];

        ns_update_p(&state);

        double abs_diff = 0;
        int id = 0;

        for (int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = state.p.data[v];
            act_fl.exp = state.p.exp;
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