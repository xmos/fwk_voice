// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_STATE_H
#define IC_STATE_H

#include "ic_defines.h"


/**
 * @page page_ic_state_h ic_state.h
 * 
 * This header contains definitions for data structures used in lib_ic.
 *
 * @ingroup ic_header_file
 */

/**
 * @defgroup ic_state   IC Data Structures
 */ 

typedef struct {
    uint8_t bypass;
    int32_t gamma_log2;
    uint32_t sigma_xx_shift;
    uint32_t coeff_index;
    fixed_s32_t ema_alpha_q30;
}ic_config_params_t;


typedef struct {
    float_s32_t smoothed_voice_chance;
    float_s32_t voice_chance_alpha;

    //Filtered versions
    float_s32_t input_energy;
    float_s32_t output_energy;
    float_s32_t energy_alpha;

    //Instantaneous versions
    float_s32_t input_energy0;
    float_s32_t output_energy0;
    float_s32_t energy_alpha0;

    int32_t vad_data_window[IC_FRAME_LENGTH];
    unsigned vad_counter;

    float_s32_t leakage_alpha;

    //This defines the ratio of the output to input at which the filter will reset.
    //A good rule is it should be above 2.0
    float_s32_t out_to_in_ratio_limit;
    float_s32_t instability_recovery_leakage_alpha;
    uint8_t enable_filter_instability_recovery;

    uint8_t enable_adaption;
} ic_adaption_controller_state_t;


typedef struct {
    bfp_s32_t y_bfp[IC_Y_CHANNELS];
    bfp_complex_s32_t Y_bfp[IC_Y_CHANNELS];
    int32_t DWORD_ALIGNED y[IC_Y_CHANNELS][IC_FRAME_LENGTH + FFT_PADDING];

    bfp_s32_t x_bfp[IC_X_CHANNELS];
    bfp_complex_s32_t X_bfp[IC_X_CHANNELS];
    int32_t DWORD_ALIGNED x[IC_X_CHANNELS][IC_FRAME_LENGTH + FFT_PADDING];

    bfp_s32_t prev_y_bfp[IC_Y_CHANNELS];
    int32_t DWORD_ALIGNED y_prev_samples[IC_Y_CHANNELS][IC_FRAME_LENGTH - IC_FRAME_ADVANCE]; //272 samples history
    bfp_s32_t prev_x_bfp[IC_X_CHANNELS];
    int32_t DWORD_ALIGNED x_prev_samples[IC_X_CHANNELS][IC_FRAME_LENGTH - IC_FRAME_ADVANCE]; //272 samples history


    bfp_complex_s32_t Y_hat_bfp[IC_Y_CHANNELS];
    bfp_s32_t y_hat_bfp[IC_Y_CHANNELS];
    complex_s32_t DWORD_ALIGNED Y_hat[IC_Y_CHANNELS][IC_FD_FRAME_LENGTH];

    bfp_complex_s32_t Error_bfp[IC_Y_CHANNELS];
    bfp_s32_t error_bfp[IC_Y_CHANNELS];
    complex_s32_t DWORD_ALIGNED Error[IC_Y_CHANNELS][IC_FD_FRAME_LENGTH];

    bfp_complex_s32_t H_hat_bfp[IC_Y_CHANNELS][IC_X_CHANNELS*IC_FILTER_PHASES];
    complex_s32_t DWORD_ALIGNED H_hat[IC_Y_CHANNELS][IC_FILTER_PHASES*IC_X_CHANNELS][IC_FD_FRAME_LENGTH];

    bfp_complex_s32_t X_fifo_bfp[IC_X_CHANNELS][IC_FILTER_PHASES];
    bfp_complex_s32_t X_fifo_1d_bfp[IC_X_CHANNELS*IC_FILTER_PHASES];
    complex_s32_t DWORD_ALIGNED X_fifo[IC_X_CHANNELS][IC_FILTER_PHASES][IC_FD_FRAME_LENGTH];

    //We reuse X memory for calculating T, see ic_init for bfp vector setup
    bfp_complex_s32_t T_bfp[IC_X_CHANNELS];

    bfp_s32_t inv_X_energy_bfp[IC_X_CHANNELS];
    int32_t DWORD_ALIGNED inv_X_energy[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];
    bfp_s32_t X_energy_bfp[IC_X_CHANNELS];
    int32_t DWORD_ALIGNED X_energy[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];
    unsigned X_energy_recalc_bin;

    bfp_s32_t overlap_bfp[IC_Y_CHANNELS];
    int32_t DWORD_ALIGNED overlap[IC_Y_CHANNELS][IC_FRAME_OVERLAP];

    int32_t y_input_delay[IC_Y_CHANNELS][IC_Y_CHANNEL_DELAY_SAMPS];
    int32_t y_delay_idx[IC_Y_CHANNELS];

    float_s32_t mu[IC_Y_CHANNELS][IC_X_CHANNELS];
    float_s32_t error_ema_energy[IC_Y_CHANNELS];
    float_s32_t max_X_energy[IC_X_CHANNELS]; 
    float_s32_t delta;

    bfp_s32_t sigma_XX_bfp[IC_X_CHANNELS];
    int32_t DWORD_ALIGNED sigma_XX[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];

    float_s32_t y_ema_energy[IC_Y_CHANNELS];
    float_s32_t x_ema_energy[IC_X_CHANNELS];
    float_s32_t overall_Y[IC_Y_CHANNELS];
    float_s32_t sum_X_energy[IC_X_CHANNELS]; 

    ic_config_params_t config_params;
    ic_adaption_controller_state_t ic_adaption_controller_state;
} ic_state_t;

#endif
