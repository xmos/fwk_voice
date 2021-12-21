// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AGC_API_H
#define AGC_API_H

#include <xs3_math.h>
#include <agc_profiles.h>

// Length of the frame of data on which the AGC will operate
#define AGC_FRAME_ADVANCE 240u

typedef struct {
    int adapt;
    int adapt_on_vad;
    int soft_clipping;
    float_s32_t gain;
    float_s32_t max_gain;
    float_s32_t min_gain;
    float_s32_t upper_threshold;
    float_s32_t lower_threshold;
    float_s32_t gain_inc;
    float_s32_t gain_dec;
    int lc_enabled;
    int lc_n_frame_far;
    int lc_n_frame_near;
    float_s32_t lc_corr_threshold;
    float_s32_t lc_bg_power_gamma;
    float_s32_t lc_gamma_inc;
    float_s32_t lc_gamma_dec;
    float_s32_t lc_far_delta;
    float_s32_t lc_near_delta;
    float_s32_t lc_near_delta_far_active;
    float_s32_t lc_gain_max;
    float_s32_t lc_gain_double_talk;
    float_s32_t lc_gain_silence;
    float_s32_t lc_gain_min;
} agc_config_t;

typedef struct {
    agc_config_t config;
    float_s32_t x_slow;
    float_s32_t x_fast;
    float_s32_t x_peak;
    int lc_t_far;
    int lc_t_near;
    float_s32_t lc_near_power_est;
    float_s32_t lc_far_power_est;
    float_s32_t lc_near_bg_power_est;
    float_s32_t lc_gain;
    float_s32_t lc_far_bg_power_est;
    float_s32_t lc_corr_val;
} agc_state_t;

void agc_init(agc_state_t *agc, agc_config_t *config);

typedef struct {
    int vad_flag;
    float_s32_t aec_ref_power;
    float_s32_t aec_corr_factor;
} agc_meta_data_t;

// Definitions that can be used for assignments in agc_meta_data_t to make it clear to those
// reading the code that the VAD or AEC are not present in the application.
// If the VAD is not present, adapt_on_vad must be set to zero in agc_config_t
#define AGC_META_DATA_NO_VAD 0u
// If the AEC is not present, lc_enabled must be set to zero in agc_config_t
#define AGC_META_DATA_NO_AEC (float_s32_t){0, 0}

void agc_process_frame(agc_state_t *agc,
                       int32_t output[AGC_FRAME_ADVANCE],
                       const int32_t input[AGC_FRAME_ADVANCE],
                       agc_meta_data_t *meta_data);

#endif
