// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// Frames of random data are created and processed by four independent instances of the AGC
// (with one input being scaled to provide low input energy for the "silence" scenario).
// Each AGC instance should get into a particular loss-control scenario: near-end speech,
// far-end speech, double-talk or silence. The AEC meta-data is specifically set for each
// AGC instance to simulate the different scenarios based on the far power and correlation
// values (which would usually come from the AEC). The actual lc_gain achieved must equal
// the expected gain for that scenario from the AGC configuration profile that is used.
// The output energy of the final frames in each case is also compared with the expected
// ordering (except "silence" as this was scaled on input).

void test_loss_control() {
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output_near[AGC_FRAME_ADVANCE];
    int32_t output_far[AGC_FRAME_ADVANCE];
    int32_t output_double_talk[AGC_FRAME_ADVANCE];
    int32_t output_silence[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp, output_near_bfp, output_far_bfp, output_double_talk_bfp;

    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);
    bfp_s32_init(&output_near_bfp, output_near, FRAME_EXP, AGC_FRAME_ADVANCE, 0);
    bfp_s32_init(&output_far_bfp, output_far, FRAME_EXP, AGC_FRAME_ADVANCE, 0);
    bfp_s32_init(&output_double_talk_bfp, output_double_talk, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    // Random seed
    unsigned seed = 38480;

    agc_state_t agc_near;
    agc_config_t conf_near = AGC_PROFILE_COMMS;
    conf_near.adapt_on_vnr = 0;

    agc_meta_data_t md_near;
    md_near.vnr_flag = AGC_META_DATA_NO_VNR;
    md_near.aec_corr_factor = f32_to_float_s32(TEST_LC_NEAR_CORR);

    agc_state_t agc_far;
    agc_config_t conf_far = AGC_PROFILE_COMMS;
    conf_far.adapt_on_vnr = 0;

    agc_meta_data_t md_far;
    md_far.vnr_flag = AGC_META_DATA_NO_VNR;
    md_far.aec_corr_factor = f32_to_float_s32(TEST_LC_FAR_CORR);

    agc_state_t agc_double_talk;
    agc_config_t conf_double_talk = AGC_PROFILE_COMMS;
    conf_double_talk.adapt_on_vnr = 0;

    agc_meta_data_t md_double_talk;
    md_double_talk.vnr_flag = AGC_META_DATA_NO_VNR;
    md_double_talk.aec_corr_factor = f32_to_float_s32(TEST_LC_DT_CORR);

    agc_state_t agc_silence;
    agc_config_t conf_silence = AGC_PROFILE_COMMS;
    conf_silence.adapt_on_vnr = 0;

    agc_meta_data_t md_silence;
    md_silence.vnr_flag = AGC_META_DATA_NO_VNR;
    md_silence.aec_corr_factor = f32_to_float_s32(TEST_LC_SILENCE_CORR);

    // Scale the input by 0.5 to avoid the AGC adaption upper threshold
    float_s32_t scale = f32_to_float_s32(0.5);
    float_s32_t scale_silence = f32_to_float_s32(TEST_LC_SILENCE_SCALE);

    unsigned num_frames = conf_near.lc_n_frame_far;
    if (num_frames < conf_near.lc_n_frame_near) {
        num_frames = conf_near.lc_n_frame_near;
    }

    for (unsigned iter = 0; iter < (1<<10)/F; ++iter) {
        agc_init(&agc_near, &conf_near);
        agc_init(&agc_far, &conf_far);
        agc_init(&agc_double_talk, &conf_double_talk);
        agc_init(&agc_silence, &conf_silence);

        for (unsigned frame = 0; frame < num_frames; ++frame) {
            for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
                input[idx] = pseudo_rand_int32(&seed);
            }
            bfp_s32_headroom(&input_bfp);
            bfp_s32_scale(&input_bfp, &input_bfp, scale);
            bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

            float_s32_t input_energy = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));

            md_near.aec_ref_power = float_s32_mul(input_energy, f32_to_float_s32(TEST_LC_NEAR_POWER_SCALE));
            agc_process_frame(&agc_near, output_near, input, &md_near);

            md_far.aec_ref_power = float_s32_mul(input_energy, f32_to_float_s32(TEST_LC_FAR_POWER_SCALE));
            agc_process_frame(&agc_far, output_far, input, &md_far);

            md_double_talk.aec_ref_power = float_s32_mul(input_energy, f32_to_float_s32(TEST_LC_DT_POWER_SCALE));
            agc_process_frame(&agc_double_talk, output_double_talk, input, &md_double_talk);

            bfp_s32_scale(&input_bfp, &input_bfp, scale_silence);
            bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

            input_energy = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));
            md_silence.aec_ref_power = float_s32_mul(input_energy, f32_to_float_s32(TEST_LC_SILENCE_POWER_SCALE));
            agc_process_frame(&agc_silence, output_silence, input, &md_silence);
        }

        TEST_ASSERT_EQUAL_FLOAT(float_s32_to_float(conf_near.lc_gain_max), float_s32_to_float(agc_near.lc_gain));
        TEST_ASSERT_EQUAL_FLOAT(float_s32_to_float(conf_far.lc_gain_min), float_s32_to_float(agc_far.lc_gain));
        TEST_ASSERT_EQUAL_FLOAT(float_s32_to_float(conf_double_talk.lc_gain_double_talk), float_s32_to_float(agc_double_talk.lc_gain));
        TEST_ASSERT_EQUAL_FLOAT(float_s32_to_float(conf_silence.lc_gain_silence), float_s32_to_float(agc_silence.lc_gain));

        bfp_s32_headroom(&output_near_bfp);
        float_s32_t output_near_energy = float_s64_to_float_s32(bfp_s32_energy(&output_near_bfp));
        bfp_s32_headroom(&output_far_bfp);
        float_s32_t output_far_energy = float_s64_to_float_s32(bfp_s32_energy(&output_far_bfp));
        bfp_s32_headroom(&output_double_talk_bfp);
        float_s32_t output_double_talk_energy = float_s64_to_float_s32(bfp_s32_energy(&output_double_talk_bfp));

        // This test assumes: lc_gain_near > lc_gain_double_talk > lc_gain_far
        TEST_ASSERT(float_s32_gt(output_near_energy, output_double_talk_energy));
        TEST_ASSERT(float_s32_gt(output_double_talk_energy, output_far_energy));
    }
}
