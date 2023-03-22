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

TEST_GROUP_RUNNER(ns_priv_update_alpha_d_tilde){
    RUN_TEST_CASE(ns_priv_update_alpha_d_tilde, case0);
}

TEST_GROUP(ns_priv_update_alpha_d_tilde);
TEST_SETUP(ns_priv_update_alpha_d_tilde) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_update_alpha_d_tilde) {}

TEST(ns_priv_update_alpha_d_tilde, case0){

    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [NS_PROC_FRAME_BINS];
    double actual;

    double alpha_d;

    int32_t p_int [NS_PROC_FRAME_BINS];
    float_s32_t p_fl;
    double p_db;

    for(int i = 0; i < 100; i++){

        ns_state_t state;
        ns_init(&state);

        alpha_d = 0.95;

        for (int v = 0; v < NS_PROC_FRAME_BINS; v++){

            p_int[v] = pseudo_rand_int(&seed, 0xffffffff, 0x7fffffff);
            p_fl.mant = p_int[v];
            p_fl.exp = EXP;
            p_db = float_s32_to_double(p_fl);

            expected[v] =  alpha_d + ((1.0 - alpha_d) * p_db);
        }

        state.p.data = &p_int[0];
        bfp_s32_headroom(&state.p);

        ns_priv_update_alpha_d_tilde(&state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
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