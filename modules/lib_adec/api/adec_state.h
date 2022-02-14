// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef __ADEC_TYPES_H__
#define __ADEC_TYPES_H__
#include "bfp_math.h"
#include "xs3_math.h"
#include "aec_defines.h"
#include "adec_defines.h"

/**
 * @page page_adec_state_h adec_state.h
 * 
 * This header contains definitions for data structures and enums used in lib_adec.
 *
 * @ingroup adec_header_file
 */

/**
 * @defgroup adec_types   ADEC Data Structure and Enum Definitions
 */

/**
 * @ingroup adec_types
 */
typedef enum {
    ADEC_NORMAL_AEC_MODE, ///< ADEC processing mode where it monitors AEC performance and requests small delay correction. 
    ADEC_DELAY_ESTIMATOR_MODE, ///< ADEC processing mode for bulk delay correction in which it measures for a new delay offset.
} adec_mode_t;

/**
 * @brief ADEC configuration structure.
 *
 * This is used to provide configuration when initialising ADEC at startup. A copy of this structure is present in the ADEC state structure
 * and available to be modified by the application for run time control of ADEC configuration.
 * 
 * @ingroup adec_types
 */
typedef struct {
    /** Bypass ADEC decision making process. When set to 1, ADEC evaluates the current input frame metrics but doesn't
     * make any delay correction or aec reset and reconfiguration requests*/
    int32_t bypass;
    /** Force trigger a delay estimation cycle. When set to 1, ADEC bypasses the ADEC monoitoring process and transitions to delay
     * estimation mode for measuring delay offset.
    */
    int32_t force_de_cycle_trigger; 
}adec_config_t;

/**
 * @brief Delay estimator output structure
 * 
 * @ingroup adec_types
 */
typedef struct {
    int32_t measured_delay_samples; ///< Estimated microphone delay in time domain samples
    int32_t peak_power_phase_index; ///< Phase index of peak energy AEC filter phase
    float_s32_t peak_phase_power; ///< Maximum per phase energy across all AEC filter phases
    float_s32_t sum_phase_powers; ///< Sum of filter energy across all filter phases.
    float_s32_t peak_to_average_ratio; ///< Ratio of peak filter phase energy to average filter phase energy. Used to evaluate how well the filter has converged.
    float_s32_t phase_power[AEC_LIB_MAX_PHASES]; ///< Phase energy of all AEC filter phases
}de_output_t;

/**
 * @brief ADEC output structure
 * 
 * @ingroup adec_types
 */
typedef struct {
    /** Flag indicating if ADEC is requesting an input delay correction*/
    int32_t delay_change_request_flag;
    /** Mic delay in samples requested by ADEC. Relevant when delay_change_request_flag is 1. Note that this value is a signed integer.
     * A positive `requested_mic_delay_samples` requires the microphone to be delayed so the application needs to delay the input mic
     * signal by `requested_mic_delay_samples` samples. A negative `requested_mic_delay_samples` means ADEC is requesting the input mic
     * signal to be moved earlier in time. This, the application should do my delaying the input reference signal 
     * by `abs(requested_mic_delay_samples)` samples.
    */
    int32_t requested_mic_delay_samples;
    /** flag indicating ADEC's request for a reset of part of the AEC state to get AEC filter to start adapting from a 0 filter.
     * ADEC requests this when a small delay correction needs to be applied that doesn't require a full reset of the AEC.*/
    
    int32_t reset_aec_flag;
    /** Flag indicating if AEC needs to be run configured in delay estimation mode.*/
    int32_t delay_estimator_enabled_flag;
    /** Requested delay samples without clamping to +- MAX_DELAY_SAMPLES. Used only for debugging.*/
    int32_t requested_delay_samples_debug;
} adec_output_t;

/**
 * @brief Input structure containing current frame's information from AEC
 * 
 * @ingroup adec_types
 */
typedef struct {
    /** EWMA energy of AEC input mic signal channel 0*/
    float_s32_t y_ema_energy_ch0;
    /** EWMA energy of AEC filter error output signal channel 0*/
    float_s32_t error_ema_energy_ch0;
    /** EWMA energy of AEC input reference signal channel 0*/
    float_s32_t x_ema_energy_ch0;
    /** shadow_flag value for the current frame computed within the AEC*/
    int32_t shadow_flag_ch0;
}aec_to_adec_t;

/**
 * @brief ADEC input structure
 * 
 * @ingroup adec_types
 */
typedef struct {
    /** ADEC input from the delay estimator*/
    de_output_t from_de;
    /** ADEC input from AEC*/
    aec_to_adec_t from_aec;
}adec_input_t;

/**
 * @brief ADEC state structure
 *
 * This structure holds the current state of the ADEC instance and members are updated each
 * time that `adec_process_frame()` runs. Many of these members are statistics from tracking the AEC performance.
 * The user should not directly modify any of these members, except the config.
 *
 * @ingroup adec_types
 */
typedef struct {
    float_s32_t max_peak_to_average_ratio_since_reset; ///< Maximum peak to average AEC filter phase energy ratio seen since a delay correction was last requested. 
    float_s32_t peak_to_average_ratio_history[ADEC_PEAK_TO_RAGE_HISTORY_DEPTH + 1]; ///< Last ADEC_PEAK_TO_RAGE_HISTORY_DEPTH frames peak_to_average_ratio of phase energies
    float_s32_t peak_power_history[ADEC_PEAK_LINREG_HISTORY_SIZE]; ///< Last ADEC_PEAK_LINREG_HISTORY_SIZE frames peak phase power
    float_s32_t aec_peak_to_average_good_aec_threshold; ///< Threshold was considering peak to average ratio as good
    
    fixed_s32_t agm_q24; ///< AEC goodness metric indicating a measure of how well AEC filter is performing
    fixed_s32_t erle_bad_bits_q24; ///< log2 of threshold below which AEC output's measured ERLE is considered bad
    fixed_s32_t erle_good_bits_q24; ///< log2 of threshold above which AEC output's measured ERLE is considered good
    fixed_s32_t peak_phase_energy_trend_gain_q24; ///< Multiplier used for scaling agm's sensitivity to peak phase energy trend 
    fixed_s32_t erle_bad_gain_q24; ///< Multiplier determining how steeply we reduce aec's goodness when measured erle falls below the bad erle threshold

    adec_mode_t mode; ///< ADEC's mode of operation. Can be operating in normal AEC or delay estimation mode
    int32_t peak_to_average_ratio_valid_flag;
    int32_t gated_milliseconds_since_mode_change; ///< milliseconds elapsed since a delay change was last requested. Used to ensure that delay corrections are not requested too early without allowing enough time for aec filter to converge.
    int32_t last_measured_delay; ///< Last measured delay 
    int32_t peak_power_history_idx; ///< index storing the head of the peak_power_history circular buffer
    int32_t peak_power_history_valid; ///< Flag indicating whether the peak_power_history buffer has been filled atleast once.
    int32_t sf_copy_flag; ///< Flag indicating if shadow to main filter copy has happened atleast once in the AEC
    int32_t convergence_counter; ///< Counter indicating number of frames the AEC shadow filter has been attempting to converge.
    int32_t shadow_flag_counter; ///< Counter indicating number of frame the AEC shadow filter has been better than the main filter.
    adec_config_t adec_config; ///< ADEC configuration parameters structure. Can be modified by application at run-time to reconfigure ADEC.
} adec_state_t;


#endif // __ADEC_TYPES_H__
