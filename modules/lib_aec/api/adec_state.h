// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __ADEC_TYPES_H__
#define __ADEC_TYPES_H__
#ifdef __XC__
extern "C" {
#endif
    #include "bfp_math.h"
    #include "xs3_math.h"
#ifdef __XC__
}
#endif
#include "adec_defines.h"

typedef enum {
    ADEC_NORMAL_AEC_MODE,
    ADEC_DELAY_ESTIMATOR_MODE,
} adec_mode_t;

typedef struct {
    int32_t requested_mic_delay_samples;
    int32_t mode_change_request_flag;
    int32_t reset_all_aec_flag;
    int32_t delay_estimator_enabled;
} adec_output_t;

typedef struct {
    int32_t peak_power_phase_index;
    float_s32_t peak_phase_power;
    float_s32_t  peak_to_average_ratio;
    uint32_t delay_estimate;
}de_to_adec_t;

typedef struct {
    float_s32_t y_ema_energy_ch0;
    float_s32_t error_ema_energy_ch0;
    int32_t shadow_better_or_equal_flag;
    int32_t shadow_to_main_copy_flag;
}aec_to_adec_t;

typedef struct {
    de_to_adec_t from_de;
    aec_to_adec_t from_aec;
    int32_t far_end_active_flag;
    int32_t num_frames_since_last_call;
    int32_t manual_de_cycle_trigger;
}adec_input_t;

typedef struct {
    float_s32_t max_peak_to_average_ratio_since_reset;
    float_s32_t peak_to_average_ratio_history[ADEC_PEAK_TO_RAGE_HISTORY_DEPTH + 1];
    float_s32_t peak_power_history[ADEC_PEAK_LINREG_HISTORY_SIZE];
    float_s32_t aec_peak_to_average_good_aec_threshold;

    fixed_s32_t agm_q24;
    fixed_s32_t erle_bad_bits_q24;
    fixed_s32_t erle_good_bits_q24;
    fixed_s32_t peak_phase_energy_trend_gain_q24;
    fixed_s32_t erle_bad_gain_q24;

    adec_mode_t mode;
    int32_t peak_to_average_ratio_valid_flag;
    int32_t enabled;
    int32_t manual_dec_cycle_trigger;
    int32_t gated_milliseconds_since_mode_change;
    int32_t last_measured_delay;
    int32_t peak_power_history_idx;
    int32_t peak_power_history_valid;
    int32_t sf_copy_flag;
    int32_t convergence_counter;
    int32_t shadow_flag_counter;
} adec_state_t;


#endif // __ADEC_TYPES_H__
