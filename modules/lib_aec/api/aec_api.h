// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_API_H
#define AEC_API_H

#include <stdio.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"

//AEC public API
/// Initialise AEC
/**
 * @brief Initialise an AEC instance
 *
 * This API initializes the AEC for a given configuration.
 * The configuration is specified in terms of
 * number of mic signal (referred to as 'y' in the API) channels,
 * number of far end audio signal (referred to as 'reference' or 'x' in the API) channels,
 * number of phases in the main filter and
 * number of phases in the shadow filter.
 * An AEC instance is made of a main filter, the error computed from which is used for the AEC output
 * and a shadow filter that is used for tracking AEC performance for a faster convergence to optimal performance.
 *
 * After a call to this API, AEC is set up to start processing frames 
 *
 *
 * @param[inout]    main_state                  AEC state structure for holding main filter specific state
 * @param[inout]    shadow_state                AEC state structure for holding shadow filter specific state
 * @params[inout]   shared_state                shared_state structure for holding state that is common to main and shadow filter
 * @param[inout]    main_mem_pool               Memory pool used by the main filter
 * @param[inout]    shadow_mem_pool             Memory pool used by the shadow filter
 * @param[out]      num_y_channels              Number of mic signal channels
 * @param[out]      num_x_channels              Number of far end audio signal channels
 * @param[out]      num_main_filter_phases      Number of phases in the main filter
 * @param[out]      num_shadow_filter_phases    Number of phases in the shadow filter
 *
 */
void aec_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        aec_shared_state_t *shared_state,
        uint8_t *main_mem_pool,
        uint8_t *shadow_mem_pool,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned num_main_filter_phases,
        unsigned num_shadow_filter_phases);

/// Sets up AEC for processing a new frame
void aec_frame_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2]);

/// Calculate average energy in time domain
void aec_update_td_ema_energy(
        float_s32_t *avg_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const aec_config_params_t *conf);

/// FFT single channel real input
void aec_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input);

/// Calculate X energy
void aec_update_total_X_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned recalc_bin);

/// Update X-fifo with the newest X data. Calculate sigmaXX
void aec_update_X_fifo_and_calc_sigmaXX(
        aec_state_t *state,
        unsigned ch);

/// Update the 1d form of X-fifo pointers
void aec_update_X_fifo_1d(
        aec_state_t *state);

/// Calculate filter Error and Y_hat
void aec_calc_Error_and_Y_hat(
        aec_state_t *state,
        unsigned ch);

/// Real IFFT to single channel input data
void aec_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        );

/// Calculate coherence
void aec_calc_coherence(
        aec_state_t *state,
        unsigned ch);

/// Window error. Overlap add to create AEC output
void aec_create_output(
        aec_state_t *state,
        unsigned ch);

/// Calculate energy in frequency domain
void aec_calc_fd_frame_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input);

/// Compare shadow and main filters and update filters if needed. Calculate mu
void aec_compare_filters_and_calc_mu(
        aec_state_t *main_state,
        aec_state_t *shadow_state);

/// Calculate inverse X-energy
void aec_calc_inv_X_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned is_shadow);

/// Calculate T (mu * inv_X_energy * Error)
void aec_compute_T(
        aec_state_t *state,
        unsigned y_ch,
        unsigned x_ch);

/// Adapt H_hat
void aec_filter_adapt(
        aec_state_t *state,
        unsigned y_ch);

/// Estimate delay
int aec_estimate_delay (
        aec_state_t *state);

//AEC level 2 API. Allows for bin or phase level parallelism

/// Calculate Error and Y_hat for a channel over a range of bins
void aec_l2_calc_Error_and_Y_hat(
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

/// Adapt one phase or 2 consecutive phases of H_hat filter
void aec_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        );

/// Unify bfp_complex_s32_t chunks into a single exponent and headroom
void aec_l2_bfp_complex_s32_unify_exponent(
        bfp_complex_s32_t *chunks,
        int *final_exp,
        int *final_hr,
        const int *mapping,
        int array_len,
        int desired_index,
        int min_headroom);

/// Unify bfp_s32_t chunks into a single exponent and headroom
void aec_l2_bfp_s32_unify_exponent(
        bfp_s32_t *chunks,
        int *final_exp,
        int *final_hr,
        const int *mapping,
        int array_len,
        int desired_index,
        int min_headroom);

//Untested API TODO add unit tests
void aec_reset_filter(
        aec_state_t *state);

#endif
