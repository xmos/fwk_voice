// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// In this test, the AGC is configured to use a fixed gain of 1, so no gain is expected to be
// applied to the incoming frame. The test generates frames of random data and processes them
// with the AGC, checking that the output is within tolerance: up to two bits can be lost in
// the BFP multiplication by the fixed gain of one that occurs inside the AGC for this test.

void test_no_gain() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];

    // Random seed
    unsigned seed = 40190;

    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_FIXED_GAIN;
    conf.gain = f32_to_float_s32(1);
    agc_init(&agc, &conf);

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }

        agc_process_frame(&agc, output, input, &md);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            TEST_ASSERT_INT32_WITHIN(3, input[idx], output[idx]);
        }
    }
}
