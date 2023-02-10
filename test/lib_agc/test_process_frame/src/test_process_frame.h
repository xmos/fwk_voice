// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AGC_UNIT_TESTS_
#define AGC_UNIT_TESTS_

#include <agc_api.h>
#include "unity.h"

#define FRAME_EXP -31

/**
 * @brief AGC profile tuned for communication with a human listener.
 *
 */
#define AGC_PROFILE_COMMS (agc_config_t){ \
    .adapt = 1, \
    .adapt_on_vnr = 1, \
    .soft_clipping = 1, \
    .gain = f32_to_float_s32(500), \
    .max_gain = f32_to_float_s32(1000), \
    .min_gain = f32_to_float_s32(0), \
    .upper_threshold = f32_to_float_s32(0.4), \
    .lower_threshold = f32_to_float_s32(0.4), \
    .gain_inc = f32_to_float_s32(1.0034), \
    .gain_dec = f32_to_float_s32(0.98804), \
    .lc_enabled = 1, \
    .lc_n_frame_far = 17, \
    .lc_n_frame_near = 34, \
    .lc_corr_threshold = f32_to_float_s32(0.993), \
    .lc_bg_power_gamma = f32_to_float_s32(1.002), \
    .lc_gamma_inc = f32_to_float_s32(1.005), \
    .lc_gamma_dec = f32_to_float_s32(0.995), \
    .lc_far_delta = f32_to_float_s32(300), \
    .lc_near_delta = f32_to_float_s32(50), \
    .lc_near_delta_far_active = f32_to_float_s32(100), \
    .lc_gain_max = f32_to_float_s32(1), \
    .lc_gain_double_talk = f32_to_float_s32(0.9), \
    .lc_gain_silence = f32_to_float_s32(0.1), \
    .lc_gain_min = f32_to_float_s32(0.022387), \
    }

// Set F to an integer greater than 1 to speedup testing (by reducing iterations) by a factor of F times
#undef F
#if SPEEDUP_FACTOR
    #define F (SPEEDUP_FACTOR)
#else
    #define F 1
#endif

// Parameters for use in loss control tests. TEST_LC_*_CORR is the meta-data aec_corr_factor, and
// TEST_LC_*_POWER_SCALE is the proportion of the input frame energy to set as the far power. Then the
// input frame will behave as in one of the scenarios: near-end, far-end, double-talk or silence.
#define TEST_LC_NEAR_CORR 0.1
#define TEST_LC_NEAR_POWER_SCALE 0.1
#define TEST_LC_FAR_CORR 0.995
#define TEST_LC_FAR_POWER_SCALE 0.95
#define TEST_LC_DT_CORR 0.5
#define TEST_LC_DT_POWER_SCALE 0.5
#define TEST_LC_SILENCE_CORR 0
#define TEST_LC_SILENCE_POWER_SCALE 0.5

// An input frame for "silence" requires low energy, so a factor is used to scale the raw input frame
// data in the silence case; for non-silence (near-end, far-end, double-talk), no scaling is required.
#define TEST_LC_NON_SILENCE_SCALE 1
#define TEST_LC_SILENCE_SCALE 0.1

#endif  // AGC_UNIT_TESTS_
