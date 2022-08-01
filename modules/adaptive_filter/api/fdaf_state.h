// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef FDAF_STATE_H
#define FDAF_STATE_H

#include <fdaf_defines.h>

typedef enum {
    IC_ADAPTION_AUTO  = 0,
    IC_ADAPTION_FORCE_ON = 1,
    IC_ADAPTION_FORCE_OFF = 2
} adaption_config_e;

typedef struct {

    uint8_t num_phases;
    int32_t gamma_log2;
    uint8_t main_filter;
    uint8_t smooth;
    float_s32_t delta;
    int32_t delta_exp;
    float_s32_t scalar_mu;
    float_s32_t * mu_ptr;

    bfp_complex_s32_t * X_fifo_ptr;
    bfp_s32_t * X_energy_ptr;
    bfp_s32_t * inv_X_energy_ptr;
    bfp_complex_s32_t * H_hat_ptr;
    bfp_complex_s32_t * Error_ptr;
    bfp_complex_s32_t * Y_hat_ptr;
}fdaf_filter_t;

typedef struct{

    uint8_t X_channel_count;
    uint8_t Y_channel_count;
    uint8_t out_channel_count;
    uint8_t sigma_xx_shift;
    float_s32_t sigma_xx_alpha;
    float_s32_t leakage_alpha;
    adaption_config_e adapt_conf;

    bfp_s32_t * x_data_ptr;
    bfp_s32_t * y_data_ptr;
    bfp_complex_s32_t * Y_data_ptr;
    bfp_s32_t * overlap_ptr;
    bfp_s32_t * sigma_xx_ptr;
}fdaf_controller;

#endif
