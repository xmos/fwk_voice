// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef FDAF_API_H
#define FDAF_API_H

void fdaf_update_total_X_energy(
        bfp_s32_t *X_energy,
        float_s32_t *max_X_energy,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *X,
        unsigned num_phases,
        unsigned recalc_bin);

void fdaf_update_X_fifo_and_calc_sigmaXX(
        bfp_complex_s32_t *X_fifo,
        bfp_s32_t *sigma_XX,
        float_s32_t *sum_X_energy,
        const bfp_complex_s32_t *X,
        unsigned num_phases,
        uint32_t sigma_xx_shift);

void fdaf_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *H_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled);

void fdaf_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph);

void fdaf_calc_inv_X_energy(
        bfp_s32_t *inv_X_energy,
        const bfp_s32_t *X_energy,
        const bfp_s32_t *sigma_XX,
        const int32_t gamma_log2,
        float_s32_t delta,
        unsigned is_shadow);

void fdaf_compute_T(
        bfp_complex_s32_t *T,
        const bfp_complex_s32_t *Error,
        const bfp_s32_t *inv_X_energy,
        float_s32_t mu);

void fdaf_calc_delta(
        float_s32_t *delta, 
        const float_s32_t *max_X_energy,
        float_s32_t delta_min,
        float_s32_t delta_adaption_force_on,
        uint8_t adapt_conf,
        float_s32_t scale,
        uint32_t channels);

#endif
