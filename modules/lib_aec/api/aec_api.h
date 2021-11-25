// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_API_H
#define AEC_API_H

#include <stdio.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"

//AEC public API
/**
 * @brief Initialise an AEC instance
 *
 * This API initializes an AEC instance for a given configuration.
 * An AEC instance comprises of a main filter, the error computed from which is used for the AEC output
 * and a shadow filter that monitors AEC performance and helps in faster convergence.
 * The configuration is specified in terms of
 * number of mic signal (referred to as 'y' in the API) channels,
 * number of far end audio signal (referred to as 'reference' or 'x' in the API) channels,
 * number of phases in the main filter and
 * number of phases in the shadow filter.
 *
 * After a call to this API, AEC is set up to start processing frames 
 *
 *
 * @param[inout]    main_state                  AEC state structure for holding main filter specific state
 * @param[inout]    shadow_state                AEC state structure for holding shadow filter specific state
 * @params[inout]   shared_state                shared_state structure for holding state that is common to main and shadow filter
 * @param[inout]    main_mem_pool               Memory pool used by the main filter
 * @param[inout]    shadow_mem_pool             Memory pool used by the shadow filter
 * @param[in]      num_y_channels              Number of mic signal channels
 * @param[in]      num_x_channels              Number of far end audio signal channels
 * @param[in]      num_main_filter_phases      Number of phases in the main filter
 * @param[in]      num_shadow_filter_phases    Number of phases in the shadow filter
 *
 * The state and shared state memory and the memory pools for both the filters must begin at double word aligned addresses.
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


/**
 * @brief Set AEC up to process a new input frame
 *
 * This function is called to set up AEC for processing a new frame. It initialises the BFP structures in AEC state with the new frame.
 * AEC works on input frame made of mic and reference channels of time domain data of length AEC_PROC_FRAME_LENGTH.
 * For processing a length AEC_PROC_FRAME_LENGTH AEC frame, application needs to allocate 2 extra samples worth of memory as part of the input memory.
 * This is required since AEC FFT transform is calculated in place and computing a real AEC_PROC_FRAME_LENGTH point FFT generates AEC_PROC_FRAME_LENGTH/2 + 1 point complex
 * output. The 2 extra samples of memory in the time domain input is used to store the Nyquist bin of the FFT output.
 *
 * @param[inout] main_state main filter state
 * @param[inout] shadow_state shadow filter state
 * @param[in] y_data mic signal
 * @param[in] x_data far end audio signal
 */
void aec_frame_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2]);

/**
 * @brief Calculate energy of a complex BFP vector.
 *
 * Computes the energy of a complex BFP vector. It statically allocates memory of type
 * int32_t and length AEC_PROC_FRAME_LENGTH/2 + 1 to use for intermediate calculations. Hence, the maximum input length
 * that is supported is AEC_PROC_FRAME_LENGTH/2 + 1. Input length more than AEC_PROC_FRAME_LENGTH/2 + 1 causes a runtime assertion
 * to trigger.
 *
 * @param[out] fd_energy energy of the complex input vector 
 * @param[in] input BFP complex input vector
 */
void aec_calc_fd_frame_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input);

/**
 * @brief Calculate exponential mean average energy of an input int32_t BFP vector
 *
 * Calculates exponential mean average energy of a BFP vector or its subset
 *
 * @param[out] ema_energy   exponential mean average of the input vector 
 * @param[in] input         int32 BFP vector
 * @param[in] start_offset  offset in the input vector from where to start calculating ema energy
 * @param[in] length        input vector subset length over which to calculate ema energy
 * @param[in] conf          AEC configuration parameters. The ema constant 'alpha' is read from here
 */
void aec_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const aec_config_params_t *conf);

/**
 * @brief Calculate forward real Discrete Fourier Transform of a real 32bit input sequence. 
 *
 * Calculates forward real DFT of a 32bit real sequence. This function performs an N-point real DFT on a 32bit real sequence where N
 * is the length of the input, to calculate N/2+1 32bit complex values as the DFT output. The N/2+1 complex output values represent frequency bins from DC up to Nyquist.
 * The DFT calculation is done in-place. After this function call the input and output BFP vectors point to the same memory for storing their data.
 * 
 * To allow for inplace transform from N real values to N/2+1 complex values, the input sequence should have 2 extra 32bit real samples
 * worth of memory. So for a input BFP vector x, an N point DFT is computed where N is x->length. x->data however points to memory of length N+2 32bit words.
 *
 * @param[out] output   32bit complex BFP DFT output
 * @param[in] input     32bit read BFP DFT input
 *
 * After this function input->data and output->data point to the same memory address.
 */
void aec_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input);

/// Calculate X energy
/**
 * @brief Calculate total energy of the X-fifo 
 *
 * This function calculates the total energy across all phases of the X-fifo for every X data bin, where X data is the
 * frequency domain representation of the far end audio input (x)
 * 
 * @param[inout] state  AEC state. 
 * @param[in]    ch     X fifo channel index for which total energy is calculated 
 * @param[in]    recalc_bin The bin index for which to properly calculate energy
 * 
 */
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
