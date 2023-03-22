// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// In this test, the AGC is configured to use the "fixed gain" profile. Frames of random data
// are processed with the AGC and the output frame energy must have increased by a factor of
// the square of the fixed gain. Also every non-zero input sample is checked to ensure that its
// magnitude has increased, and input samples of zero must be unchanged.

void test_simple_gain() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp, output_bfp;

    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);
    bfp_s32_init(&output_bfp, output, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    // Random seed
    unsigned seed = 57195;

    agc_state_t agc;
    agc_init(&agc, &AGC_PROFILE_FIXED_GAIN);

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Scale down the input so that the fixed gain doesn't overflow
    float_s32_t scale = float_s32_div(f32_to_float_s32(1), agc.config.gain);

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }
        bfp_s32_headroom(&input_bfp);
        bfp_s32_scale(&input_bfp, &input_bfp, scale);
        bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

        float_s32_t input_energy = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));

        // expected_output_energy = input_energy * agc.config.gain * agc.config.gain
        float_s32_t expected_output_energy = input_energy;
        expected_output_energy = float_s32_mul(expected_output_energy, agc.config.gain);
        expected_output_energy = float_s32_mul(expected_output_energy, agc.config.gain);

        agc_process_frame(&agc, output, input, &md);

        bfp_s32_headroom(&output_bfp);
        float_s32_t output_energy = float_s64_to_float_s32(bfp_s32_energy(&output_bfp));

        TEST_ASSERT_EQUAL_FLOAT(float_s32_to_float(expected_output_energy), float_s32_to_float(output_energy));

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            if (input[idx] > 0) {
                TEST_ASSERT_GREATER_THAN_INT32(input[idx], output[idx]);
            } else if (input[idx] < 0) {
                TEST_ASSERT_LESS_THAN_INT32(input[idx], output[idx]);
            } else {
                TEST_ASSERT_EQUAL_INT32(0, output[idx]);
            }
        }
    }
}
