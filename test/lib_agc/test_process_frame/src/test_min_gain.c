// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// The AGC is configured with a gain that is less than the minimum gain setting. The
// upper_threshold and lower_threshold are set to extremes to avoid interfering with
// the test. A frame of random input data (scaled to avoid overflow) is processed by
// the AGC and the output is checked to ensure that the minimum gain has been applied.

#define TEST_GAIN 50
#define TEST_MIN_GAIN 100
#if TEST_MIN_GAIN < TEST_GAIN
#error "gain must be less than min_gain for this test"
#endif

void test_min_gain() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp;
    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_ASR;
    conf.adapt_on_vnr = 0;
    conf.soft_clipping = 0;
    conf.lc_enabled = 0;
    conf.min_gain = f32_to_float_s32(TEST_MIN_GAIN);
    // Set the upper and lower thresholds to extremes to avoid interfering
    conf.lower_threshold = f32_to_float_s32(0);
    conf.upper_threshold = f32_to_float_s32(1);
    agc_init(&agc, &conf);

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Random seed
    unsigned seed = 9608;

    // Scale the input down by the min_gain so there is room to increase it fully
    float_s32_t scale = float_s32_div(f32_to_float_s32(1), conf.min_gain);

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        // Reset the gain for each frame
        agc.config.gain = f32_to_float_s32(TEST_GAIN);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }
        bfp_s32_headroom(&input_bfp);
        bfp_s32_scale(&input_bfp, &input_bfp, scale);
        bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

        agc_process_frame(&agc, output, input, &md);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            TEST_ASSERT_EQUAL_INT32(input[idx] * TEST_MIN_GAIN, output[idx]);
        }

        // Also check the configured gain parameter has been clamped as expected
        TEST_ASSERT_EQUAL_INT32(agc.config.min_gain.mant, agc.config.gain.mant);
        TEST_ASSERT_EQUAL_INT(agc.config.min_gain.exp, agc.config.gain.exp);
    }
}
