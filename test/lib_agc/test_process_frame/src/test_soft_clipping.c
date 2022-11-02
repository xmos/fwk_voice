// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// Frames of random data are generated (scaled to avoid overflow), and processed by two
// instances of the AGC: one has soft_clipping enabled, the other has it disabled. The
// outputs are compared to ensure that soft-clipping has been applied.

void test_soft_clipping() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output_clip[AGC_FRAME_ADVANCE];
    int32_t output_no_clip[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp;
    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    agc_state_t agc_clip;
    agc_config_t conf_clip = AGC_PROFILE_ASR;
    conf_clip.adapt_on_vnr = 0;
    conf_clip.soft_clipping = 1;
    conf_clip.lc_enabled = 0;
    agc_init(&agc_clip, &conf_clip);

    agc_state_t agc_no_clip;
    agc_config_t conf_no_clip = AGC_PROFILE_ASR;
    conf_no_clip.adapt_on_vnr = 0;
    conf_no_clip.soft_clipping = 0;
    conf_no_clip.lc_enabled = 0;
    agc_init(&agc_no_clip, &conf_no_clip);

    // Meta-data is constant and can be shared by the two AGC instances
    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Random seed
    unsigned seed = 62809;

    // Scale the input down so that there is enough room to apply the max gain
    float_s32_t scale = float_s32_div(f32_to_float_s32(1), conf_clip.max_gain);
    float_s32_t zero = f32_to_float_s32(0);
    // This is the threshold above which soft-clipping is applied
    float_s32_t thresh = f32_to_float_s32(0.5);

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }
        bfp_s32_headroom(&input_bfp);
        bfp_s32_scale(&input_bfp, &input_bfp, scale);
        bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

        agc_process_frame(&agc_clip, output_clip, input, &md);

        agc_process_frame(&agc_no_clip, output_no_clip, input, &md);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            float_s32_t input_fl = {input[idx], FRAME_EXP};
            float_s32_t output_no_clip_fl = {output_no_clip[idx], FRAME_EXP};

            if (float_s32_gte(input_fl, zero) && float_s32_gt(output_no_clip_fl, thresh)) {
                // Positive sample, greater than thresh
                TEST_ASSERT_GREATER_OR_EQUAL_INT32(output_clip[idx], output_no_clip[idx]);
            } else if (float_s32_gt(zero, input_fl) && float_s32_gt(float_s32_sub(zero, thresh), output_no_clip_fl)) {
                // Negative sample, less than -thresh
                TEST_ASSERT_LESS_OR_EQUAL_INT32(output_clip[idx], output_no_clip[idx]);
            } else {
                // No soft-clipping should be applied
                TEST_ASSERT_EQUAL_INT32(output_clip[idx], output_no_clip[idx]);
            }
        }
    }
}
