// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// A single iteration of this test generates frames of random data and processes them with
// the AGC. Within a certain number of frames, the AGC is expected to adapt to get the
// maximum sample of the frame above the configured lower_threshold. Then the remaining
// number of frames in the test are processed to ensure that the maximum sample remains above
// that threshold. The AGC is reset for each test iteration.

// Number of frames allowed for the AGC to get the sample below the threshold
#define MAX_ADAPT_FRAMES 20
// Total number of frames to test
#define MAX_TEST_FRAMES (MAX_ADAPT_FRAMES + 10)

void test_lower_threshold() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp, output_bfp;
    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);
    bfp_s32_init(&output_bfp, output, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_ASR;
    conf.adapt_on_vnr = 0;
    conf.lc_enabled = 0;

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Random seed
    unsigned seed = 11533;

    // Max gain is 1000, so scale the input by a factor larger than 1/1000 from the
    // lower_threshold which the AGC is trying to exceed.
    float_s32_t scale = float_s32_mul(f32_to_float_s32(0.0011), conf.lower_threshold);

    float_s32_t lower_threshold;
    if(conf.soft_clipping){ //Soft clipping limits gain a bit at top end
        lower_threshold = float_s32_mul(conf.lower_threshold, f32_to_float_s32(0.90));
    }else{
        lower_threshold = conf.lower_threshold;
    }

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        unsigned frame;

        agc_init(&agc, &conf);

        for (frame = 0; frame < MAX_ADAPT_FRAMES; ++frame) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }
            bfp_s32_headroom(&input_bfp);
            bfp_s32_scale(&input_bfp, &input_bfp, scale);
            bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

            agc_process_frame(&agc, output, input, &md);

            bfp_s32_headroom(&output_bfp);
            bfp_s32_abs(&output_bfp, &output_bfp);
            float_s32_t max = bfp_s32_max(&output_bfp);

            if (float_s32_gte(max, lower_threshold)) {
                break;
            }
        }

        TEST_ASSERT(frame < MAX_ADAPT_FRAMES);

        for (; frame < MAX_TEST_FRAMES; ++frame) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }
            bfp_s32_headroom(&input_bfp);
            bfp_s32_scale(&input_bfp, &input_bfp, scale);
            bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

            agc_process_frame(&agc, output, input, &md);

            bfp_s32_headroom(&output_bfp);
            bfp_s32_abs(&output_bfp, &output_bfp);
            float_s32_t max = bfp_s32_max(&output_bfp);

            TEST_ASSERT_FALSE(float_s32_gt(lower_threshold, max));
        }
    }
}
