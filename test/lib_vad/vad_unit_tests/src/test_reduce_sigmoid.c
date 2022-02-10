// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_nn_coefficients.h"
#include "ai.h"
 

extern void vad_reduce_sigmoid(int32_t activated[], const int64_t raw_layer[], const size_t N);

void test_dummy() {
    unsigned int seed = 6031769;

    for(int iter=0; iter<(1<<11)/F; iter++) {

        int64_t ref_raw[N_VAD_HIDDEN] = {0};

        for(int i=0; i<N_VAD_HIDDEN;i++){
            ref_raw[i] = pseudo_rand_int32(&seed) + ((int64_t)pseudo_rand_int32(&seed) << 32) ;
        }

        int32_t ref_result[N_VAD_HIDDEN] = {0};
        int32_t dut_result[N_VAD_HIDDEN] = {0};

        nn_reduce_relu(ref_result, ref_raw, N_VAD_HIDDEN);

        vad_reduce_sigmoid(dut_result, ref_raw, N_VAD_HIDDEN);

        for(int i=0; i<N_VAD_HIDDEN;i++){
            // printf("%d - ref: %ld dut: %ld diff:%ld\n", i, ref_result[i], dut_result[i], ref_result[i] - dut_result[i]);
            const int32_t delta = 1; //31bits of equivalence 
            TEST_ASSERT_INT32_WITHIN(delta, ref_result[i], dut_result[i]);
        }
    }
}