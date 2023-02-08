// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// A single iteration of this test generates frames of random data and processes them with
// the AGC. Within a certain number of frames, the AGC is expected to adapt to get the
// maximum sample of the frame within the configured upper_threshold. Then the remaining
// number of frames in the test are processed to ensure that the samples remain below that
// threshold. The AGC is reset for each test iteration.

// Number of frames allowed for the AGC to get the sample below the threshold
#define MAX_ADAPT_FRAMES 10
// Total number of frames to test
#define MAX_TEST_FRAMES (MAX_ADAPT_FRAMES + 30)

void test_upper_threshold() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];
    bfp_s32_t output_bfp;
    bfp_s32_init(&output_bfp, output, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_ASR;
    conf.adapt_on_vnr = 0;
    conf.lc_enabled = 0;

    // Set initial gain to a lower value to save time adapting
    conf.gain = f32_to_float_s32(1);

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Random seed
    unsigned seed = 16395;

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        unsigned frame;

        agc_init(&agc, &conf);

        for (frame = 0; frame < MAX_ADAPT_FRAMES; ++frame) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }

            agc_process_frame(&agc, output, input, &md);

            bfp_s32_headroom(&output_bfp);
            bfp_s32_abs(&output_bfp, &output_bfp);
            float_s32_t max = bfp_s32_max(&output_bfp);

            if (float_s32_gte(agc.config.upper_threshold, max)) {
                break;
            }
        }

        TEST_ASSERT(frame < MAX_ADAPT_FRAMES);

        for (; frame < MAX_TEST_FRAMES; ++frame) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }

            agc_process_frame(&agc, output, input, &md);

            bfp_s32_headroom(&output_bfp);
            bfp_s32_abs(&output_bfp, &output_bfp);
            float_s32_t max = bfp_s32_max(&output_bfp);

            TEST_ASSERT_FALSE(float_s32_gt(max, agc.config.upper_threshold));
        }
    }
}
