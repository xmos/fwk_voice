// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// A number of frames of random data are processed by the AGC, with the first output frame
// saved for later. The AGC is then "reset" by performing the initialisation again. Then
// the first input frame is processed again to ensure that it matches the output frame that
// was saved from earlier.

// Number of frames to process before resetting
#define NUM_FRAMES 10

void test_reset() {
    int32_t input0[AGC_FRAME_ADVANCE];
    int32_t output0[AGC_FRAME_ADVANCE];
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];

    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_ASR;
    conf.adapt_on_vnr = 0;
    conf.lc_enabled = 0;

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;
    md.aec_ref_power = AGC_META_DATA_NO_AEC;
    md.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Random seed
    unsigned seed = 8895;

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        agc_init(&agc, &conf);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input0[idx] = pseudo_rand_int32(&seed);
        }

        agc_process_frame(&agc, output0, input0, &md);

        for (unsigned frames = 1; frames < NUM_FRAMES; ++frames) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }

            agc_process_frame(&agc, output, input, &md);
        }

        agc_init(&agc, &conf);

        agc_process_frame(&agc, output, input0, &md);

        TEST_ASSERT_EQUAL_INT32_ARRAY(output, output0, AGC_FRAME_ADVANCE);
    }
}
