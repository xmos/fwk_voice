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
#define LUT_SIZE 64
#define LUT_INPUT_MULTIPLIER 4
const int32_t LUT_TEST[LUT_SIZE] = {
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1139054005, 883582904, 685409774, 531683621,
412435719, 319933163, 248177410, 192515294,
149337276, 115843378, 89861612, 69707128,
54072965, 41945287, 32537648, 25239988,
19579073, 15187809, 11781433, 9139051,
7089312, 5499296, 4265894, 3309124,
2566942, 1991219, 1544621, 1198188,
929454, 720993, 559286, 433847,
336543, 261062, 202510, 157090,
121857, 94527, 73326, 56880,
44123, 34227, 26550, 20596,
15976, 12393, 9614, 7457
};

TEST_GROUP_RUNNER(ns_priv_subtract_lambda_from_frame){
    RUN_TEST_CASE(ns_priv_subtract_lambda_from_frame, case0);
}

TEST_GROUP(ns_priv_subtract_lambda_from_frame);
TEST_SETUP(ns_priv_subtract_lambda_from_frame) { fflush(stdout); }
TEST_TEAR_DOWN(ns_priv_subtract_lambda_from_frame) {}

TEST(ns_priv_subtract_lambda_from_frame, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    double expected [NS_PROC_FRAME_BINS];
    double actual;

    double abs_Y_db [NS_PROC_FRAME_BINS];
    int32_t abs_Y_int [NS_PROC_FRAME_BINS];
    float_s32_t abs_Y_fl;

    double lambda_db [NS_PROC_FRAME_BINS];
    int32_t lambda_int [NS_PROC_FRAME_BINS];
    float_s32_t lambda_fl;

    double r_data_db;
    int32_t r_data_int;
    float_s32_t r_data_fl;

    for(int i = 0; i < 100; i++){

        ns_state_t state;
        ns_init(&state);

        int lut_index;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
            abs_Y_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            abs_Y_fl.mant = abs_Y_int[v];
            abs_Y_fl.exp = EXP;
            abs_Y_db[v] = float_s32_to_double(abs_Y_fl);

            lambda_int[v] = pseudo_rand_int(&seed, 0x10000000, 0x7fffffff);
            lambda_fl.mant = lambda_int[v];
            lambda_fl.exp = EXP;
            lambda_db[v] = float_s32_to_double(lambda_fl);

            lut_index = sqrt(lambda_db[v]) / LUT_INPUT_MULTIPLIER;
            lut_index = abs_Y_db[v] / lut_index;

            r_data_int = (lut_index > (LUT_SIZE - 1)) ? 0 : LUT_TEST[lut_index];
            r_data_fl.mant = r_data_int;
            r_data_fl.exp = EXP;
            r_data_db = float_s32_to_double(r_data_fl);

            expected[v] = (sqrt(lambda_db[v]) > abs_Y_db[v]) ? abs_Y_db[v] : sqrt(lambda_db[v]);
            expected[v] *= r_data_db;
            expected[v] = abs_Y_db[v] - expected[v];
            if(expected[v] < 0) expected[v] = 0;
        }

        bfp_s32_t abs_Y_bfp;
        bfp_s32_init(&abs_Y_bfp, abs_Y_int, EXP, NS_PROC_FRAME_BINS, 1);
        state.lambda_hat.data = &lambda_int[0];
        bfp_s32_headroom(&state.lambda_hat);

        ns_priv_subtract_lambda_from_frame(&abs_Y_bfp, &state);

        double abs_diff = 0;
        int id = 0;

        for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
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
        double thresh = ldexp(1, -26);
        TEST_ASSERT(rel_error < thresh);
    }
}