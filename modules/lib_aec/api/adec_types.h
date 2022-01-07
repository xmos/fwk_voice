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
#include "aec_delay_estimator_control_parameters.h"

typedef enum {
    ADEC_NORMAL_AEC_MODE,
    ADEC_DELAY_ESTIMATOR_MODE,
} adec_mode_t;

typedef enum {
    ADEC_NO_MODE_CHANGE_NEEDED = 0,
    ADEC_MODE_CHANGE_REQUESTED
} adec_delay_change_t;

typedef enum {
    ADEC_PK_AVE_INVALID = 0,
    ADEC_PK_AVE_VALID
} adec_pk_ave_valid_t;


typedef struct {
    int32_t requested_mic_delay_samples;
    adec_delay_change_t mode_change_request; 
    uint32_t reset_all_aec_flag;
    uint32_t delay_estimator_enabled;
} adec_output_t;

typedef struct {
    int32_t peak_power_phase_index;
    float_s32_t peak_phase_power;
    float_s32_t  peak_to_average_ratio;
    uint32_t delay_estimate;
}delay_estimator_output_t;

typedef struct {
    float_s32_t erle_ratio;
    int shadow_flag;
}aec_to_adec_t;

typedef struct {
    fixed_s32_t agm_q24;
    fixed_s32_t erle_bad_bits_q24;
    fixed_s32_t erle_good_bits_q24;
    fixed_s32_t peak_phase_energy_trend_gain_q24;
    fixed_s32_t erle_bad_gain_q24;

    float_s32_t max_peak_to_average_ratio_since_reset;
    float_s32_t peak_to_average_ratio_history[ADEC_PEAK_TO_RAGE_HISTORY_DEPTH + 1];
    float_s32_t peak_power_history[ADEC_PEAK_LINREG_HISTORY_SIZE];
    float_s32_t aec_peak_to_average_good_aec_threshold;

    adec_pk_ave_valid_t peak_to_average_ratio_valid;
    adec_mode_t mode;
    unsigned enabled;
    unsigned manual_dec_cycle_trigger;
    uint32_t gated_milliseconds_since_mode_change;
    int32_t last_measured_delay;
    unsigned peak_power_history_idx;
    unsigned peak_power_history_valid;
    unsigned sf_copy_flag;
    unsigned convergence_counter;
    unsigned shadow_flag_counter;
} adec_state_t;


#endif // __ADEC_TYPES_H__
