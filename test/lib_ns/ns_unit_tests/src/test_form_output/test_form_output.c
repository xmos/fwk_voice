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

TEST_GROUP_RUNNER(ns_priv_form_output){
    RUN_TEST_CASE(ns_priv_form_output, case0);
}

TEST_GROUP(ns_priv_form_output);
TEST_SETUP(ns_priv_form_output) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_form_output) {}

TEST(ns_priv_form_output, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();

    int32_t overlap_int[NS_FRAME_ADVANCE];
    int32_t frame_int[NS_PROC_FRAME_LENGTH];
    int32_t output[NS_FRAME_ADVANCE];
    float_s32_t frame_fl;
    float_s32_t overlap_fl;
    float_s32_t t;
    double ex_overlap[NS_FRAME_ADVANCE];
    double ex_output[NS_FRAME_ADVANCE];
    double frame_db[NS_PROC_FRAME_LENGTH];
    double act_output;
    double act_overlap;

    for(int i = 0; i < 100; i++){
        
        bfp_s32_t overlap, frame;
        bfp_s32_init(&overlap, overlap_int, EXP, NS_FRAME_ADVANCE, 0);
        bfp_s32_init(&frame, frame_int, EXP, NS_PROC_FRAME_LENGTH, 0);

        for(int v = 0; v < NS_FRAME_ADVANCE; v++){
            overlap_int[v] = pseudo_rand_int(&seed, 0, INT_MAX)/2;
            overlap_fl.mant = overlap_int[v];
            overlap_fl.exp = EXP;
            ex_overlap[v] = float_s32_to_double(overlap_fl);
        }
        bfp_s32_headroom(&overlap);

        for(int j = 0; j < 5; j++){
            
            for(int v = 0; v < NS_PROC_FRAME_LENGTH; v++){
                frame_int[v] = pseudo_rand_int(&seed, INT_MIN, INT_MAX)/2;
                frame_fl.mant = frame_int[v];
                frame_fl.exp = EXP;
                frame_db[v] = float_s32_to_double(frame_fl);
            }
            bfp_s32_headroom(&frame);

            ns_priv_form_output(output, &frame, &overlap);

            for(int v = 0; v < NS_FRAME_ADVANCE; v++){
                ex_output[v] = frame_db[v] + ex_overlap[v];
                ex_overlap[v] = frame_db[v + NS_FRAME_ADVANCE];
                t.mant = overlap.data[v];
                t.exp = overlap.exp;
            }
        }

        double abs_diff = 0;
        int id = 0;

        for (int v = 0; v < NS_FRAME_ADVANCE; v++){
            float_s32_t act_fl;
            act_fl.mant = output[v];
            act_fl.exp = EXP;
            act_output = float_s32_to_double(act_fl);

            double t = fabs(ex_output[v] - act_output);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        double rel_error = fabs(abs_diff/(ex_output[id] + ldexp(1, -40)));
        double thresh = ldexp(1, -29);
        TEST_ASSERT(rel_error < thresh);

        abs_diff = 0;
        id = 0;

        for (int v = 0; v < NS_FRAME_ADVANCE; v++){
            float_s32_t act_fl;
            act_fl.mant = overlap.data[v];
            act_fl.exp = EXP;
            act_overlap = float_s32_to_double(act_fl);

            double t = fabs(ex_overlap[v] - act_overlap);

            if (t > abs_diff){
                abs_diff = t; 
                id = v;
            }
        }

        rel_error = fabs(abs_diff/(ex_output[id] + ldexp(1, -40)));
        thresh = ldexp(1, -29);
        TEST_ASSERT(rel_error < thresh);
    }
}