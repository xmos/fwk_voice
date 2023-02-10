// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "xmath/xmath.h"

#include <ns_api.h>
#include <ns_priv.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31

TEST_GROUP_RUNNER(ns_priv_update_p){
    RUN_TEST_CASE(ns_priv_update_p, case0);
}

TEST_GROUP(ns_priv_update_p);
TEST_SETUP(ns_priv_update_p) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_update_p) {}

TEST(ns_priv_update_p, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();

    double alpha_p;
    double delta;

    int32_t S_int [NS_PROC_FRAME_BINS];
    int32_t S_min_int [NS_PROC_FRAME_BINS];
    int32_t p_int [NS_PROC_FRAME_BINS];
    float_s32_t S_fl;
    float_s32_t S_min_fl;
    float_s32_t p_fl;
    double S_db;
    double S_min_db;

    double expected [NS_PROC_FRAME_BINS];
    double actual;

    for(int i = 0; i < 100; i++){

        ns_state_t state;
        ns_init(&state);
        
        alpha_p = 0.2;
        delta = 1.5;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            
            S_int[v] = pseudo_rand_int(&seed, 0x1bbbbbbb, 0x7fffffff);
            S_fl.mant = S_int[v];
            S_fl.exp = EXP;
            S_db = float_s32_to_double(S_fl);

            S_min_int[v] = pseudo_rand_int(&seed, 0x11111111, 0x1ddddddd);
            S_min_fl.mant = S_min_int[v];
            S_min_fl.exp = EXP;
            S_min_db = float_s32_to_double(S_min_fl);

            p_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
            p_fl.mant = p_int[v];
            p_fl.exp = EXP;
            expected[v] = float_s32_to_double(p_fl);

            expected[v] *= alpha_p;

            if((S_db / S_min_db) > delta){
                expected[v] += (1.0 - alpha_p);

            }
        }

        state.S.data = &S_int[0];
        bfp_s32_headroom(&state.S);
        state.S_min.data = &S_min_int[0];
        bfp_s32_headroom(&state.S_min);
        state.p.data = &p_int[0];
        bfp_s32_headroom(&state.p);

        ns_priv_update_p(&state);

        double abs_diff = 0;
        int id = 0;

        for (int v = 0; v < NS_PROC_FRAME_BINS; v++){
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
        double thresh = ldexp(1, -20);
        TEST_ASSERT(rel_error < thresh);
    }
}