// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AGC_PROFILES_H
#define AGC_PROFILES_H

#include "xmath/xmath.h"

/**
 * @page page_agc_profiles_h agc_profiles.h
 *
 * This header contains pre-defined profiles for AGC configurations.
 * These profiles can be used to initialise the `agc_config_t` data
 * for use with `agc_init()`.
 *
 * This header is automatically included by `agc_api.h`.
 */

/**
 * @defgroup agc_profiles   Pre-defined AGC configuration profiles
 */

/**
 * @brief AGC profile tuned for Automatic Speech Recognition (ASR).
 *
 * @ingroup agc_profiles
 */
#define AGC_PROFILE_ASR (agc_config_t){ \
    .adapt = 1, \
    .adapt_on_vnr = 1, \
    .soft_clipping = 1, \
    .gain = f32_to_float_s32(500), \
    .max_gain = f32_to_float_s32(1000), \
    .min_gain = f32_to_float_s32(0), \
    .upper_threshold = f32_to_float_s32(0.9999), \
    .lower_threshold = f32_to_float_s32(0.7000), \
    .gain_inc = f32_to_float_s32(1.197), \
    .gain_dec = f32_to_float_s32(0.87), \
    .lc_enabled = 0, \
    .lc_n_frame_far = 0, \
    .lc_n_frame_near = 0, \
    .lc_corr_threshold = f32_to_float_s32(0), \
    .lc_bg_power_gamma = f32_to_float_s32(0), \
    .lc_gamma_inc = f32_to_float_s32(0), \
    .lc_gamma_dec = f32_to_float_s32(0), \
    .lc_far_delta = f32_to_float_s32(0), \
    .lc_near_delta = f32_to_float_s32(0), \
    .lc_near_delta_far_active = f32_to_float_s32(0), \
    .lc_gain_max = f32_to_float_s32(0), \
    .lc_gain_double_talk = f32_to_float_s32(0), \
    .lc_gain_silence = f32_to_float_s32(0), \
    .lc_gain_min = f32_to_float_s32(0), \
    }

/**
 * @brief AGC profile tuned to apply a fixed gain.
 *
 * @ingroup agc_profiles
 */
#define AGC_PROFILE_FIXED_GAIN (agc_config_t){ \
    .adapt = 0, \
    .adapt_on_vnr = 0, \
    .soft_clipping = 0, \
    .gain = f32_to_float_s32(25), \
    .max_gain = f32_to_float_s32(0), \
    .min_gain = f32_to_float_s32(0), \
    .upper_threshold = f32_to_float_s32(0), \
    .lower_threshold = f32_to_float_s32(0), \
    .gain_inc = f32_to_float_s32(0), \
    .gain_dec = f32_to_float_s32(0), \
    .lc_enabled = 0, \
    .lc_n_frame_far = 0, \
    .lc_n_frame_near = 0, \
    .lc_corr_threshold = f32_to_float_s32(0), \
    .lc_bg_power_gamma = f32_to_float_s32(0), \
    .lc_gamma_inc = f32_to_float_s32(0), \
    .lc_gamma_dec = f32_to_float_s32(0), \
    .lc_far_delta = f32_to_float_s32(0), \
    .lc_near_delta = f32_to_float_s32(0), \
    .lc_near_delta_far_active = f32_to_float_s32(0), \
    .lc_gain_max = f32_to_float_s32(0), \
    .lc_gain_double_talk = f32_to_float_s32(0), \
    .lc_gain_silence = f32_to_float_s32(0), \
    .lc_gain_min = f32_to_float_s32(0), \
    }


#endif
