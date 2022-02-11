// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_nn_coefficients.h"
#include "ai.h"
 

extern void vad_fc_layer(  int64_t output[], const size_t num_out,
                    const int32_t input[], const size_t num_in,
                    const int32_t weights[]);


#define N_COEFFS ((1 + VAD_N_FEATURES) * N_VAD_HIDDEN)

void test_dummy() {
    unsigned int seed = 6031759;

    int32_t coeffs[N_COEFFS]={0};
    for(int i=0; i<N_COEFFS; i++){ 
        coeffs[i] = pseudo_rand_int32(&seed) % 0xffff;   
    }

    int32_t nn_features[VAD_N_FEATURES] = {0};


    for(int iter=0; iter<(1<<11)/F; iter++) {
        for(int i=0; i<VAD_N_FEATURES;i++){
            nn_features[i] = pseudo_rand_int32(&seed);
        }

        int64_t ref_result[N_VAD_HIDDEN] = {0};
        int64_t dut_result[N_VAD_HIDDEN] = {0};

        nn_layer_fc(ref_result,     N_VAD_HIDDEN,
                    nn_features,    N_VAD_INPUTS,
                    coeffs);

        vad_fc_layer(   dut_result,         N_VAD_HIDDEN,
                        nn_features,        N_VAD_INPUTS,
                        coeffs);

        for(int i=0; i<N_VAD_HIDDEN;i++){
            // printf("%d - ref: %lld dut: %lld diff:%lld\n", i, ref_result[i], dut_result[i], ref_result[i] - dut_result[i]);
            const int64_t delta = (1LL << 34); //30 bits of equivalence 
            TEST_ASSERT_INT64_WITHIN(delta, ref_result[i], dut_result[i]);
        }
    }
}