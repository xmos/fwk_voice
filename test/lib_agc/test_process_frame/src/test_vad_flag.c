// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include <bfp_math.h>
#include <pseudo_rand.h>

// Take an input frame of random data and run it through two AGC instances: one always sets
// the VAD indicator to true, the other always to false. The VAD output should be greater
// than the non-VAD output for a non-zero input sample; an input sample of zero should be
// unchanged.

void test_vad_flag() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output_vad0[AGC_FRAME_ADVANCE];
    int32_t output_vad1[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp;

    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    // Random seed
    unsigned seed = 62336;

    agc_state_t agc_vad0;
    agc_config_t conf_vad0 = AGC_PROFILE_COMMS;
    // Set the upper and lower threshold to one so that AGC adaption with VAD always gains
    conf_vad0.lower_threshold = float_to_float_s32(1);
    conf_vad0.upper_threshold = float_to_float_s32(1);
    conf_vad0.lc_enabled = 0;
    agc_init(&agc_vad0, &conf_vad0);

    agc_meta_data_t md_vad0;
    md_vad0.vad_flag = 0;
    md_vad0.aec_ref_power = AGC_META_DATA_NO_AEC;
    md_vad0.aec_corr_factor = AGC_META_DATA_NO_AEC;

    agc_state_t agc_vad1;
    agc_config_t conf_vad1 = AGC_PROFILE_COMMS;
    // Set the upper and lower threshold to one so that AGC adaption with VAD always gains
    conf_vad1.lower_threshold = float_to_float_s32(1);
    conf_vad1.upper_threshold = float_to_float_s32(1);
    conf_vad1.lc_enabled = 0;
    agc_init(&agc_vad1, &conf_vad1);

    agc_meta_data_t md_vad1;
    md_vad1.vad_flag = 1;
    md_vad1.aec_ref_power = AGC_META_DATA_NO_AEC;
    md_vad1.aec_corr_factor = AGC_META_DATA_NO_AEC;

    // Scale the input to allow room to apply the max gain
    float_s32_t scale = float_s32_div(float_to_float_s32(1), conf_vad0.max_gain);

    for (unsigned iter = 0; iter < (1<<12)/F; ++iter) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }
        bfp_s32_headroom(&input_bfp);
        bfp_s32_scale(&input_bfp, &input_bfp, scale);
        bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

        agc_process_frame(&agc_vad0, output_vad0, input, &md_vad0);

        agc_process_frame(&agc_vad1, output_vad1, input, &md_vad1);

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            if (input[idx] > 0) {
                TEST_ASSERT_GREATER_THAN_INT32(output_vad0[idx], output_vad1[idx]);
            } else if (input[idx] < 0) {
                TEST_ASSERT_LESS_THAN_INT32(output_vad0[idx], output_vad1[idx]);
            } else {
                TEST_ASSERT_EQUAL_INT32(output_vad0[idx], output_vad1[idx]);
            }
        }
    }
}
