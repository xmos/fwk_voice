// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "test_process_frame.h"
#include "xmath/xmath.h"
#include <pseudo_rand.h>

// Frames of random data are processed by an AGC instance, which has its meta-data set to
// transition to a particular loss-control scenario: near-end speech, far-end speech,
// double-talk or silence. The loss-control is expected to transition to the new state
// within a fixed number of frames.
//
// Transitions between every pair of states is tested, except:
//   - near-end to far-end
//   - double-talk to far-end
// These direct transitions aren't possible because a period of silence is required to move
// to the far-end state, otherwise the gain would produce a bad experience during double-talk
// because it would keep switching between near-end and far-end on the gaps between words.

// Expect the LC state to transition within this number of frames
#define TRANSITION_FRAMES 50

struct lc_test_params {
    float correlation;    // The aec_corr_factor to set in the AGC meta-data
    float power_scale;    // Proportion of the total frame energy that is set as the far power
    float silence_scale;  // Factor to scale the input frame as "silence" requires a small input
};

#define PARAMS_NEAR (struct lc_test_params){ \
    .correlation = TEST_LC_NEAR_CORR, \
    .power_scale = TEST_LC_NEAR_POWER_SCALE, \
    .silence_scale = TEST_LC_NON_SILENCE_SCALE \
    }

#define PARAMS_FAR (struct lc_test_params){ \
    .correlation = TEST_LC_FAR_CORR, \
    .power_scale = TEST_LC_FAR_POWER_SCALE, \
    .silence_scale = TEST_LC_NON_SILENCE_SCALE \
    }

#define PARAMS_DOUBLE_TALK (struct lc_test_params){ \
    .correlation = TEST_LC_DT_CORR, \
    .power_scale = TEST_LC_DT_POWER_SCALE, \
    .silence_scale = TEST_LC_NON_SILENCE_SCALE \
    }

#define PARAMS_SILENCE (struct lc_test_params){ \
    .correlation = TEST_LC_SILENCE_CORR, \
    .power_scale = TEST_LC_SILENCE_POWER_SCALE, \
    .silence_scale = TEST_LC_SILENCE_SCALE \
    }

// Random seed
unsigned seed = 30289;

static void perform_transition(agc_state_t *agc, struct lc_test_params *params, float_s32_t expected)
{
    int32_t input[AGC_FRAME_ADVANCE];
    int32_t output[AGC_FRAME_ADVANCE];
    bfp_s32_t input_bfp;

    bfp_s32_init(&input_bfp, input, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    agc_meta_data_t md;
    md.vnr_flag = AGC_META_DATA_NO_VNR;

    // Scale input frame by 0.5 to avoid AGC adaption upper threshold
    float_s32_t scale = f32_to_float_s32(0.5 * params->silence_scale);

    for (unsigned frame = 0; frame < TRANSITION_FRAMES; ++frame) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            input[idx] = pseudo_rand_int32(&seed);
        }
        bfp_s32_headroom(&input_bfp);
        bfp_s32_scale(&input_bfp, &input_bfp, scale);
        bfp_s32_use_exponent(&input_bfp, FRAME_EXP);

        float_s32_t input_energy = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));

        md.aec_ref_power = float_s32_mul(input_energy, f32_to_float_s32(params->power_scale));
        md.aec_corr_factor = f32_to_float_s32(params->correlation);
        agc_process_frame(agc, output, input, &md);

        // Return here if successfully transitioned to the expected state
        if ((agc->lc_gain.mant == expected.mant) && (agc->lc_gain.exp == expected.exp)) {
            return;
        }
    }

    // Have failed to transition to the expected state
    TEST_ASSERT(0);
}

void test_lc_transitions() {
    agc_state_t agc;
    agc_config_t conf = AGC_PROFILE_COMMS;
    conf.adapt_on_vnr = 0;

    for (unsigned iter = 0; iter < (1<<10)/F; ++iter) {
        agc_init(&agc, &conf);

        // Far-end only
        perform_transition(&agc, &PARAMS_FAR, conf.lc_gain_min);

        // Silence
        perform_transition(&agc, &PARAMS_SILENCE, conf.lc_gain_silence);

        // Double-talk
        perform_transition(&agc, &PARAMS_DOUBLE_TALK, conf.lc_gain_double_talk);

        // Silence
        perform_transition(&agc, &PARAMS_SILENCE, conf.lc_gain_silence);

        // Near-end only
        perform_transition(&agc, &PARAMS_NEAR, conf.lc_gain_max);

        // Silence
        perform_transition(&agc, &PARAMS_SILENCE, conf.lc_gain_silence);

        // Far-end only
        perform_transition(&agc, &PARAMS_FAR, conf.lc_gain_min);

        // Double-talk
        perform_transition(&agc, &PARAMS_DOUBLE_TALK, conf.lc_gain_double_talk);

        // Near-end only
        perform_transition(&agc, &PARAMS_NEAR, conf.lc_gain_max);

        // Double-talk
        perform_transition(&agc, &PARAMS_DOUBLE_TALK, conf.lc_gain_double_talk);

        // Silence
        perform_transition(&agc, &PARAMS_SILENCE, conf.lc_gain_silence);

        // Far-end only
        perform_transition(&agc, &PARAMS_FAR, conf.lc_gain_min);

        // Near-end only
        perform_transition(&agc, &PARAMS_NEAR, conf.lc_gain_max);
    }
}
