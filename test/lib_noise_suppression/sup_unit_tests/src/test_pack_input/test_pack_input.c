// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <bfp_math.h>
#include <math.h>

#include <suppression.h>
#include <suppression_testing.h>
#include <unity.h>

#include "unity_fixture.h"
#include <pseudo_rand.h>
#include <testing.h>

#define EXP  -31

TEST_GROUP_RUNNER(sup_pack_input){
    RUN_TEST_CASE(sup_pack_input, case0);
}

TEST_GROUP(sup_pack_input);
TEST_SETUP(sup_pack_input) { fflush(stdout); }
TEST_TEAR_DOWN(sup_pack_input) {}

TEST(sup_pack_input, case0){
    unsigned seed = SEED_FROM_FUNC_NAME();
    int32_t ex_curr [SUP_PROC_FRAME_LENGTH];
    int32_t ex_prev [SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)];

    int32_t input_int [SUP_FRAME_ADVANCE];
    float_s32_t input_fl;
    

    for(int i = 0; i < 1; i++){
        bfp_s32_t curr;
        bfp_s32_t prev;
        int32_t curr_data[SUP_PROC_FRAME_LENGTH];
        int32_t prev_data[SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)];

        bfp_s32_init(&curr, curr_data, EXP, SUP_PROC_FRAME_LENGTH, 0);
        bfp_s32_init(&prev, prev_data, EXP, (SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)), 0);

        for(int v = 0; v < SUP_PROC_FRAME_LENGTH; v++){
            prev_data[v] = 0;
            ex_prev[v] = 0;
        }

        for(int j = 0; j < 5; j++){

            for(int v = 0; v < (SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE); v++){
                ex_curr[v] = prev_data[v];
            }

            for(int v = 0; v < SUP_FRAME_ADVANCE; v++){
                input_int[v] = pseudo_rand_int(&seed, 0, INT_MAX);
                ex_curr[v + SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE] = input_int[v];
            }

            for(int v = 0; v < (SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)); v++){
                ex_prev[v] = ex_prev[v + SUP_FRAME_ADVANCE];
            }

            for(int v = 0; v < SUP_FRAME_ADVANCE; v ++){
                ex_prev[v + SUP_PROC_FRAME_LENGTH - (2 * SUP_FRAME_ADVANCE)] = input_int[v];
            }
            sup_pack_input(&curr, input_int, &prev);
        }

        for(int v = 0; v < SUP_PROC_FRAME_LENGTH; v++){
            TEST_ASSERT_EQUAL(ex_curr[v], curr.data[v]);
        }

        for(int v = 0; v < (SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE); v++){
            TEST_ASSERT_EQUAL(ex_prev[v], prev.data[v]);
        }
    }
}