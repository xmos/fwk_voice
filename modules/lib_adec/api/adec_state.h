// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __ADEC_TYPES_H__
#define __ADEC_TYPES_H__
#include "bfp_math.h"
#include "xs3_math.h"
#include "adec_defines.h"

typedef enum {
    ADEC_NORMAL_AEC_MODE,
    ADEC_DELAY_ESTIMATOR_MODE,
} adec_mode_t;

typedef struct {
    int32_t bypass;
    int32_t force_de_cycle_trigger; 
}adec_config_params_t;

typedef struct {
    int32_t delay_change_request_flag; ///< ADEC is requesting a change in the delay applied at the input
    int32_t requested_mic_delay_samples; ///< Mic delay in samples requested by ADEC. Relevant when delay_change_request_flag is 1
    /** flag indicating ADEC requesting an AEC reset. This is one when there's a delay
    change requested that is not accompanied by an AEC mode change. A change in AEC mode would anyway reinitialise the
    AEC so no need to do an extra reset*/
    int32_t reset_all_aec_flag; 
    int32_t delay_estimator_enabled_flag; ///< Flag indicating whether or not AEC should run in delay estimator mode
} adec_output_t;

typedef struct {
    int32_t peak_power_phase_index;
    float_s32_t peak_phase_power;
    float_s32_t  peak_to_average_ratio;
    uint32_t delay_estimate;
}de_to_adec_t;

typedef struct {
    float_s32_t y_ema_energy_ch0; //for erle
    float_s32_t error_ema_energy_ch0; //for erle
    int32_t shadow_better_or_equal_flag; //not passing shadow_flag to avoid sharing shadow_state_e between lib_aec and lib_adec
    int32_t shadow_to_main_copy_flag;
}aec_to_adec_t;

typedef struct {
    de_to_adec_t from_de;
    aec_to_adec_t from_aec;
    int32_t far_end_active_flag;
    int32_t num_frames_since_last_call;
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
    int32_t gated_milliseconds_since_mode_change;
    int32_t last_measured_delay;
    int32_t peak_power_history_idx;
    int32_t peak_power_history_valid;
    int32_t sf_copy_flag;
    int32_t convergence_counter;
    int32_t shadow_flag_counter;
    /** Measured delay samples in ADEC. Put in stage so that the app can access it for logging. Needs to be persistant
     * since logged every frame but updated only when a delay change is detected, so can't be put in adec_output_t*/
    int32_t measured_delay_samples_debug;
    adec_config_params_t adec_config;
} adec_state_t;


#endif // __ADEC_TYPES_H__
