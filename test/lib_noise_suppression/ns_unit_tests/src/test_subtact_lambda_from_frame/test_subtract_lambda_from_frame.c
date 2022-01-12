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
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31
#define LUT_SIZE 64
#define LUT_INPUT_MULTIPLIER 4
const int32_t LUT_TEST[LUT_SIZE] = {
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2278108010,  1767165807,  1370819547,  1063367242,  
824871438,  639866325,  496354819,  385030587,  
298674552,  231686756,  179723223,  139414256,  
108145929,  83890573,  65075296,  50479976,  
39158146,  30375617,  23562865,  18278102,  
14178624,  10998591,  8531788,  6618248,  
5133883,  3982438,  3089242,  2396376,  
1858908,  1441985,  1118572,  867694,  
673085,  522123,  405019,  314180,  
243714,  189053,  146651,  113760,  
88245,  68453,  53100,  41191,  
31952,  24786,  19227,  14914,  
};

TEST_GROUP_RUNNER(ns_subtract_lambda_from_frame){
    RUN_TEST_CASE(ns_subtract_lambda_from_frame, case0);
}

TEST_GROUP(ns_subtract_lambda_from_frame);
TEST_SETUP(ns_subtract_lambda_from_frame) { fflush(stdout); }
TEST_TEAR_DOWN(ns_subtract_lambda_from_frame) {}

TEST(ns_subtract_lambda_from_frame, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [SUP_PROC_FRAME_BINS];
    double actual;

    double abs_Y_db [SUP_PROC_FRAME_BINS];
    int32_t abs_Y_int [SUP_PROC_FRAME_BINS];
    float_s32_t abs_Y_fl;

    double lambda_db [SUP_PROC_FRAME_BINS];
    int32_t lambda_int [SUP_PROC_FRAME_BINS];
    float_s32_t lambda_fl;

    for(int i = 0; i < 100; i++){

        sup_state_t state;
        sup_init_state(&state);

        float_s32_t t;
        int32_t min = INT_MAX;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            abs_Y_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            abs_Y_fl.mant = abs_Y_int[v];
            abs_Y_fl.exp = EXP;
            abs_Y_db[v] = float_s32_to_double(abs_Y_fl);

            lambda_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            lambda_fl.mant = lambda_int[v];
            lambda_fl.exp = EXP;
            lambda_db[v] = float_s32_to_double(lambda_fl);

            expected[v] = sqrt(lambda_db[v]) / (double)LUT_INPUT_MULTIPLIER;
            expected[v] = abs_Y_db[v] / (expected[v] + ldexp(1, -50));

            if ((int32_t)expected[v] < min){min = (int32_t)expected[v];}
        }
        if(min < (LUT_SIZE - 1)){t.mant = min; t.exp = EXP;}
        else{t.mant = LUT_SIZE - 1; t.exp = 0;}

        t.mant = LUT_TEST[(int32_t)float_s32_to_double(t)];
        t.exp = EXP;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            expected[v] = abs_Y_db[v] - (sqrt(lambda_db[v]) * float_s32_to_double(t));
            if(expected[v] < 0){expected[v] = 0;}
        }

        bfp_s32_t abs_Y_bfp;
        bfp_s32_init(&abs_Y_bfp, abs_Y_int, EXP, SUP_PROC_FRAME_BINS, 1);
        state.lambda_hat.data = &lambda_int[0];
        bfp_s32_headroom(&state.lambda_hat);

        ns_subtract_lambda_from_frame(&abs_Y_bfp, &state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
            float_s32_t act_fl;
            act_fl.mant = abs_Y_bfp.data[v];
            act_fl.exp = abs_Y_bfp.exp;
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