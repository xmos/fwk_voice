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
 * @brief Initialise AEC memory and data structures
 *
 * This function initializes AEC data structures for a given configuration.
 * The configuration parameters num_y_channels, num_x_channels, num_main_filter_phases and num_shadow_filter_phases are passed in as input arguments.
 * This function needs to be called at startup to initialise a new AEC and subsequently whenever the AEC configuration changes.
 *
 * @param[inout]    main_state                  AEC state structure for holding main filter specific state
 * @param[inout]    shadow_state                AEC state structure for holding shadow filter specific state
 * @param[inout]   shared_state                shared_state structure for holding state that is common to main and shadow filter
 * @param[inout]    main_mem_pool               Memory pool containing main filter memory buffers
 * @param[inout]    shadow_mem_pool             Memory pool containing shadow filter memory buffers
 * @param[in]      num_y_channels              Number of mic signal (also referred as `y` in the API) channels
 * @param[in]      num_x_channels              Number of far end audio signal (also referred to as `x` or `reference` in the API) channels
 * @param[in]      num_main_filter_phases      Number of phases in the main filter
 * @param[in]      num_shadow_filter_phases    Number of phases in the shadow filter
 *
 * main_state, shadow_state and shared_state structures must start at double word aligned addresses.
 *
 * main_mem_pool and shadow_mem_pool must point to memory buffers big enough to support main and shadow filter processing. Refer to <test/lib_aec/shared_src/aec_memory_pool.h> when statically allocating memory pool or call aec_get_memory_pool_size() to get sizes in case of dynamic allocation.
 *
 * main_mem_pool and shadow_mem_pool must also start at double word aligned addresses.
 *
 * @par Example
 * @code{.c}
 *      #include "aec_memory_pool.h"
        aec_state_t DWORD_ALIGNED main_state;
        aec_state_t DWORD_ALIGNED shadow_state;
        aec_shared_state_t DWORD_ALIGNED aec_shared_state;
        uint8_t DWORD_ALIGNED aec_mem[sizeof(aec_memory_pool_t)];
        uint8_t DWORD_ALIGNED aec_shadow_mem[sizeof(aec_shadow_filt_memory_pool_t)];
        unsigned y_chans = 2, x_chans = 2;
        unsigned main_phases = 10, shadow_phases = 10;
        aec_init(&main_state, &shadow_state, &shared_state, aec_mem, aec_shadow_mem, y_chans, x_chans, main_phases, shadow_phases);
 * @endcode
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
 * @brief Initialise AEC data structures for processing a new frame
 *
 * This function is called as the first thing when a new frame needs to be processed. It initialises AEC data structures with the new frame.
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
 * @brief Calculate energy of a frequency domain (FD) sequence.
 *
 * This function calculates the energy of frequency domain data used in the AEC. Frequency domain data in AEC is in the form of complex int32 vectors of length AEC_PROC_FRAME_LENGTH/2 + 1
 *
 * It statically allocates scratch memory of type int32_t and length AEC_PROC_FRAME_LENGTH/2 + 1 to use for intermediate calculations.
 *
 * @param[out] fd_energy energy of the complex input vector 
 * @param[in] input BFP complex structure for the input
 */
void aec_calc_fd_frame_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input);

/**
 * @brief Calculate exponential moving average (EMA) energy of a time domain (TD) sequence
 *
 * This function calculates the EMA energy of time domain data used in the AEC. Time domain data is stored in AEC in the form of int32 vectors of length AEC_PROC_FRAME_LENGTH
 *
 * This function can be called to calculate the EMA energy of the entire vector or of its subsets. 
 *
 * @param[out] ema_energy   EMA energy of the input time domain data
 * @param[in] input         int32 BFP structure for the time domain data
 * @param[in] start_offset  offset in the input vector from where to start calculating ema energy
 * @param[in] length        length over which to calculate EMA energy
 * @param[in] conf          AEC configuration parameters. The EMA constant 'alpha' is read from here
 */
void aec_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const aec_config_params_t *conf);

/**
 * @brief Calculate forward Discrete Fourier Transform (DFT) of AEC time domain sequence
 *
 * This function performs a AEC_PROC_FRAME_LENGTH point real DFT. 
 * The input to this function is a length AEC_PROC_FRAME_LENGTH int32 vector. 
 * The function outputs a AEC_PROC_FRAME_LENGTH/2+1 length, int32 complex vector. The AEC_PROC_FRAME_LENGTH/2+1 complex output values represent frequency domain samples from DC up to Nyquist.
 *
 * The DFT calculation is done in-place. After this function call the input and output vectors point to the same memory.
 *
 * @param[out] output   complex int32 DFT input BFP structure
 * @param[in] input     int32 DFT output BFP structure
 *
 * To allow for inplace transform from N int32 values to N/2+1 complex int32 values, the input sequence should have 2 extra 32bit int32 samples worth of memory.
 * This means that input->data should point to a buffer of length input->length+2
 *
 * After this function input->data and output->data point to the same memory address.
 */
void aec_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input);

/**
 * @brief Calculate inverse Discrete Fourier Transform (DFT) of an AEC frequency domain sequence
 *
 * This function calculates a AEC_PROC_FRAME_LENGTH point real inverse DFT.
 * The input to this function is a length AEC_PROC_FRAME_LENGTH/2+1 complex int32 vector which represents frequency domain data samples from DC upto Nyquist.
 * The function outputs a AEC_PROC_FRAME_LENGTH length int32 vector which represents time domain samples.
 *
 * The inverse DFT is performed in place. After this function the input and output vectors point to the same memory.
 *
 *  @param[out] output int32 inverse DFT output BFP structure
 *  @param[in] input complex int32 inverse DFT input BFP structure
 *
 *  After this function input->data and output->data point to the same memory address
 */
void aec_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        );

/**
 * @brief Calculate total energy for a given channel of the X-fifo 
 *
 * X is the frequency domain reprsentation of the far end audio input for the current frame.
 * X-fifo is a FIFO of the num_main_filter_phases most recent X frames
 *
 * This function calculates the energy per X sample summed across all phases of the X-fifo
 * 
 * @param[inout] state  AEC state. state->X_energy and state->shared_state->sum_X_energy is updated 
 * @param[in]    ch     channel index for which total energy is calculated 
 * @param[in]    recalc_bin The sample index for which to recalculate energy to eliminate quantisation errors
 * 
 * recalc_bin needs to keep cycling through indexes 0 to AEC_PROC_FRAME_LENGTH/2+1
 */
void aec_update_total_X_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned recalc_bin);

/**
 * @brief Update X-FIFO with the latest X frame
 *
 * X-FIFO is a FIFO of the num_main_filter_phases most recent X frames. This function updates the X-FIFO by removing the oldest X frame and adding the current X frame to the FIFO. It also calculates sigmaXX which is the EMA of the total energy across all samples of the current X frame. 
 *
 * @param[inout] state AEC state structure. state->shared_state->X_FIFO and state->shared_state->sigma_XX are updated.
 * @param[in] ch far end audio channel index for which to update X-FIFO
 */
void aec_update_X_fifo_and_calc_sigmaXX(
        aec_state_t *state,
        unsigned ch);

/**
 * @brief Calculate AEC filter Error and Y_hat
 *
 * Y_hat is the frequency domain estimate of the far end audio present in the input microphone signal. Y_hat is calculated by convolving the X-FIFO with H_hat, where X-FIFO is a FIFO of 'num_phases' most recent X frames and H_hat is the estimated room response filter.
 *
 * Error is the frequency domain representation of what is left what is left after far end echo is removed from the input microphone signal. It is computed by subtracting Y_hat from Y
 *
 * @param[inout] state AEC state structure. state->Error and state->Y_hat are updated
 * @param[in] ch mic channel index for which to compute Error and Y_hat
 */
void aec_calc_Error_and_Y_hat(
        aec_state_t *state,
        unsigned ch);

/**
 * @brief Calculate coherence
 *
 * This function calculates the average coherence between the time domain microphone signal (y) and time domain estimate of the far end echo present in the microphone signal (y-hat). It outputs two values, the exponential moving average coherence and the slow moving exponential moving average coherence.
 *
 * @param[inout] state AEC state structure. state->shared_state->coh_mu_state[ch].coh and state->shared_state->coh_mu_state[ch].coh_slow are updated
 * @param[in] ch mic channel index for which to calculate coherence
 */
void aec_calc_coherence(
        aec_state_t *state,
        unsigned ch);

/// Update the 1d form of X-fifo pointers
void aec_update_X_fifo_1d(
        aec_state_t *state);



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
