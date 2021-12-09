// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef NEW_AEC_DEFINES_H
#define NEW_AEC_DEFINES_H

#include <stdio.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"

#define AEC_LIB_MAX_Y_CHANNELS (2)
#define AEC_LIB_MAX_X_CHANNELS (2)
#define AEC_LIB_MAIN_FILTER_PHASES (10)
#define AEC_LIB_SHADOW_FILTER_PHASES (5)

#define AEC_LIB_MAX_PHASES (AEC_LIB_MAX_Y_CHANNELS * AEC_LIB_MAX_X_CHANNELS * AEC_LIB_MAIN_FILTER_PHASES)
#define AEC_PROC_FRAME_LENGTH (512)
#define AEC_FRAME_ADVANCE (240)
#define UNUSED_TAPS_PER_PHASE (16)

typedef enum {
    AEC_ADAPTION_AUTO,
    AEC_ADAPTION_FORCE_ON,
    AEC_ADAPTION_FORCE_OFF,
} aec_adaption_e;

typedef enum {
    LOW_REF = -4,    // Not much reference so no point in acting on AEC filter logic
    ERROR = -3,      // something has gone wrong, zero shadow filter
    ZERO = -2,       // shadow filter has been reset multiple times, zero shadow filter
    RESET = -1,      // copy main filter to shadow filter
    EQUAL = 0,       // main filter and shadow filter are similar
    SIGMA = 1,       // shadow filter bit better than main, reset sigma_xx for faster convergence
    COPY = 2,        // shadow filter much better, copy to main
}shadow_state_e;

typedef struct {
    float_s32_t coh_alpha;
    float_s32_t coh_slow_alpha;
    float_s32_t coh_thresh_slow;
    float_s32_t coh_thresh_abs;
    float_s32_t mu_scalar;
    float_s32_t eps;
    float_s32_t thresh_minus20dB;
    float_s32_t x_energy_thresh;

    unsigned mu_coh_time;
    unsigned mu_shad_time;
    aec_adaption_e adaption_config;
    int32_t force_adaption_mu_q30;
} coherence_mu_config_params_t;

typedef struct {
    float_s32_t coh;
    float_s32_t coh_slow;

    int32_t mu_coh_count;
    int32_t mu_shad_count;
    float_s32_t coh_mu[AEC_LIB_MAX_X_CHANNELS];
}coherence_mu_params_t;

typedef struct {
    float_s32_t shadow_sigma_thresh;
    float_s32_t shadow_copy_thresh;
    float_s32_t shadow_reset_thresh;
    float_s32_t shadow_delay_thresh;
    float_s32_t x_energy_thresh;
    float_s32_t shadow_mu;

    int32_t shadow_better_thresh;
    int32_t shadow_zero_thresh;
    int32_t shadow_reset_timer;
}shadow_filt_config_params_t;

typedef struct {
    int32_t shadow_flag[AEC_LIB_MAX_Y_CHANNELS];
    int shadow_reset_count[AEC_LIB_MAX_Y_CHANNELS];
    int shadow_better_count[AEC_LIB_MAX_Y_CHANNELS];
}shadow_filter_params_t;

typedef struct {
    int32_t peak_power_phase_index;
    float_s32_t peak_phase_power;
    float_s32_t sum_phase_powers;
    float_s32_t peak_to_average_ratio;
    float_s32_t phase_power[AEC_LIB_MAX_X_CHANNELS * AEC_LIB_MAX_PHASES];
}delay_estimator_params_t;

typedef struct {
    int bypass;
    int gamma_log2;
    uint32_t sigma_xx_shift;
    float_s32_t delta_adaption_force_on;
    float_s32_t delta_min;
    uint32_t coeff_index;
    fixed_s32_t ema_alpha_q30;
}aec_core_config_params_t;

typedef struct{
    int32_t window_mem[32];
    int32_t window_flpd_mem[32];
    bfp_s32_t window;
    bfp_s32_t window_flpd;
}aec_window_t;

typedef struct {
    coherence_mu_config_params_t coh_mu_conf;
    shadow_filt_config_params_t shadow_filt_conf;
    aec_core_config_params_t aec_core_conf;
    aec_window_t aec_window;
}aec_config_params_t;

typedef struct {
    bfp_complex_s32_t X_fifo[AEC_LIB_MAX_X_CHANNELS][AEC_LIB_MAX_PHASES]; //TODO add comment about AEC_LIB_MAX_PHASES instead of AEC_LIB_MAIN_FILTER_PHASES
    bfp_complex_s32_t X[AEC_LIB_MAX_X_CHANNELS];
    bfp_complex_s32_t Y[AEC_LIB_MAX_Y_CHANNELS]; //Not shared between shadow and main since Error is computed inplace

    bfp_s32_t y[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t x[AEC_LIB_MAX_X_CHANNELS];
    bfp_s32_t sigma_XX[AEC_LIB_MAX_X_CHANNELS]; //Have a copy for shadow filter though not needed for a cleaner aec_calc_inv_X_energy API

    float_s32_t y_ema_energy[AEC_LIB_MAX_Y_CHANNELS];
    float_s32_t x_ema_energy[AEC_LIB_MAX_X_CHANNELS];
    float_s32_t overall_Y[AEC_LIB_MAX_Y_CHANNELS];
    float_s32_t sum_X_energy[AEC_LIB_MAX_X_CHANNELS]; 

    coherence_mu_params_t coh_mu_state[AEC_LIB_MAX_Y_CHANNELS];
    shadow_filter_params_t shadow_filter_params;
    delay_estimator_params_t delay_estimator_params;
    aec_config_params_t config_params; //control params
    unsigned num_y_channels;
    unsigned num_x_channels;
}aec_shared_state_t;

typedef struct {
    bfp_complex_s32_t Y_hat[AEC_LIB_MAX_Y_CHANNELS];
    bfp_complex_s32_t Error[AEC_LIB_MAX_Y_CHANNELS];
    bfp_complex_s32_t H_hat[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_X_CHANNELS*AEC_LIB_MAX_PHASES];
    bfp_complex_s32_t X_fifo_1d[AEC_LIB_MAX_X_CHANNELS*AEC_LIB_MAX_PHASES];
    bfp_complex_s32_t T[AEC_LIB_MAX_X_CHANNELS];

    bfp_s32_t inv_X_energy[AEC_LIB_MAX_X_CHANNELS];
    bfp_s32_t X_energy[AEC_LIB_MAX_X_CHANNELS];
    bfp_s32_t output[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t overlap[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t y_hat[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t error[AEC_LIB_MAX_Y_CHANNELS];

    float_s32_t mu[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_X_CHANNELS];
    float_s32_t error_ema_energy[AEC_LIB_MAX_Y_CHANNELS];
    float_s32_t overall_Error[AEC_LIB_MAX_Y_CHANNELS]; 
    float_s32_t max_X_energy[AEC_LIB_MAX_X_CHANNELS]; 
    float_s32_t delta_scale;
    float_s32_t delta;

    aec_shared_state_t *shared_state; //pointer to the state shared between shadow and main filter
    unsigned num_phases;
}aec_state_t;

#if !PROFILE_PROCESSING
    #define prof(n, str)
    #define print_prof(start, end, framenum)
#endif

#endif
