// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef IC_LOW_LEVEL_H
#define IC_LOW_LEVEL_H

#include "ic_state.h"

// IC Private API

// Setup core configuration parameters of the IC
void ic_priv_init_config_params(ic_config_params_t *config_params);

void ic_adaption_controller_init(ic_adaption_controller_state_t *state);

// Sets IC mu and leakage depending on VNR probability
void ic_mu_control_system(
        ic_state_t *state, 
        float_s32_t vnr);

// Calculates fast energy
void ic_calc_fast_ratio(ic_adaption_controller_state_t * ad_state);

// Adapt H_hat
void ic_filter_adapt(ic_state_t *state);

// Some nice doxygen comments
int32_t ic_init(ic_state_t *state);

// Delays y_channel w.r.t. x_channel
void ic_delay_y_input(ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE]);

// Sets up IC for processing a new frame
void ic_frame_init(
        ic_state_t *state,
        int32_t y_data[IC_FRAME_LENGTH+2],
        int32_t x_data[IC_FRAME_LENGTH+2]);

// Calculate average energy in time domain
void ic_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const uq2_30 alpha);

// FFT single channel real input
void ic_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input);

// Calculate X energy
void ic_update_X_energy(
        ic_state_t *state,
        unsigned ch,
        unsigned recalc_bin);

// Update X-fifo with the newest X data. Calculate sigmaXX
void ic_update_X_fifo_and_calc_sigmaXX(
        ic_state_t *state,
        unsigned ch);

// Update the 1d form of X-fifo pointers
void ic_update_X_fifo_1d(
        ic_state_t *state);

// Calculate filter Error and Y_hat
void ic_calc_Error_and_Y_hat(
        ic_state_t *state,
        unsigned ch);

// Real IFFT to single channel input data
void ic_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        );

// Window error. Overlap add to create IC output
void ic_create_output(
        ic_state_t *state,
        int32_t output[IC_FRAME_ADVANCE],
        unsigned ch);


// Calculate inverse X-energy
void ic_calc_inv_X_energy(
        ic_state_t *state,
        unsigned ch);

// Calculate T (mu * inv_X_energy * Error)
void ic_compute_T(
        ic_state_t *state,
        unsigned y_ch,
        unsigned x_ch);

// Calculate Error and Y_hat for a channel over a range of bins
void ic_l2_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *H_hat,
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled);

// Adapt one phase or 2 consecutive phases of H_hat filter
void ic_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        );

// Unify bfp_complex_s32_t chunks into a single exponent and headroom
void ic_l2_bfp_complex_s32_unify_exponent(
        bfp_complex_s32_t *chunks,
        int *final_exp,
        int *final_hr,
        const int *mapping,
        int array_len,
        int desired_index,
        int min_headroom);

// Unify bfp_s32_t chunks into a single exponent and headroom
void ic_l2_bfp_s32_unify_exponent(
        bfp_s32_t *chunks,
        int *final_exp,
        int *final_hr,
        const int *mapping,
        int array_len,
        int desired_index,
        int min_headroom);

// Clear coefficients to zero
void ic_reset_filter(ic_state_t *state, int32_t output[IC_FRAME_ADVANCE]);

// Leak H_hat to forget adaption
void ic_apply_leakage(
        ic_state_t *state,
        unsigned y_ch);
#endif
