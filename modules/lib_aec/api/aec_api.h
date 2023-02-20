// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_API_H
#define AEC_API_H

#include <stdio.h>
#include <string.h>
#include "xmath/xmath.h"
#include "aec_state.h"

/**
 * @page page_aec_api_h aec_api.h
 * 
 * lib_aec public functions API.
 *
 * @ingroup aec_header_file
 */

/**
 * @defgroup aec_func     High Level API Functions
 * @defgroup aec_low_level_func   Low Level API Functions (STILL WIP)
 */ 

/**
 * @brief Initialise AEC data structures
 *
 * This function initializes AEC data structures for a given configuration.
 * The configuration parameters num_y_channels, num_x_channels, num_main_filter_phases and num_shadow_filter_phases are
 * passed in as input arguments.
 *
 * This function needs to be called at startup to first initialise the AEC and subsequently whenever the AEC configuration changes.
 *
 * @param[inout] main_state               AEC state structure for holding main filter specific state
 * @param[inout] shadow_state             AEC state structure for holding shadow filter specific state
 * @param[inout] shared_state             Shared state structure for holding state that is common to main and shadow filter
 * @param[inout] main_mem_pool            Memory pool containing main filter memory buffers
 * @param[inout] shadow_mem_pool          Memory pool containing shadow filter memory buffers
 * @param[in] num_y_channels              Number of mic input channels
 * @param[in] num_x_channels              Number of reference input channels
 * @param[in] num_main_filter_phases      Number of phases in the main filter
 * @param[in] num_shadow_filter_phases    Number of phases in the shadow filter
 *
 * `main_state`, `shadow_state` and shared_state structures must start at double word aligned addresses.
 *
 * main_mem_pool and shadow_mem_pool must point to memory buffers big enough to support main and shadow filter
 * processing.  AEC state aec_state_t and shared state aec_shared_state_t structures contain only the BFP data
 * structures used in the AEC. The memory these BFP structures will point to needs to be provided by the user in the
 * memory pool main and shadow filters memory pool. An example memory pool structure is present in aec_memory_pool_t and
 * aec_shadow_filt_memory_pool_t.
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
        unsigned main_phases = 10, shadow_phases = 5;
        // There is one main and one shadow filter per x-y channel pair, so for this example there will be 4 main and 4
        // shadow filters. Each main filter will have 10 phases and each shadow filter will have 5 phases.
        aec_init(&main_state, &shadow_state, &shared_state, aec_mem, aec_shadow_mem, y_chans, x_chans, main_phases, shadow_phases);
 * @endcode
 *
 * @ingroup aec_func
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
 * This is the first function that is called when a new frame is available for processing.
 * It takes the new samples as input and combines the new samples and previous frame's history to create a processing block on which further processing happens.
 * It also initialises some data structures that need to be initialised at the beginning of a frame.
 *
 * @param[inout] main_state main filter state
 * @param[inout] shadow_state shadow filter state
 * @param[in] y_data pointer to mic input buffer
 * @param[in] x_data pointer to reference input buffer
 *
 * @note
 * @parblock
 * y_data and x_data buffers memory is free to be reused after this function call.
 * @endparblock
 *
 * @ingroup aec_func
 */
void aec_frame_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE]);

/**
 * @brief Calculate energy in the spectrum
 *
 * This function calculates the energy of frequency domain data used in the AEC. Frequency domain data in AEC is in the form of complex 32bit vectors and energy is calculated as the squared magnitude of the input vector.
 *
 * @param[out] fd_energy energy of the input spectrum
 * @param[in] input input spectrum BFP structure
 *
 * @ingroup aec_func
 */
void aec_calc_freq_domain_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input);

/**
 * @brief Calculate exponential moving average (EMA) energy of a time domain (TD) vector
 *
 * This function calculates the EMA energy of AEC time domain data which is in the form of real 32bit vectors.
 *
 * This function can be called to calculate the EMA energy of subsets of the input vector as well. 
 *
 * @param[out] ema_energy   EMA energy of the input
 * @param[in] input         time domain input BFP structure
 * @param[in] start_offset  offset in the input vector from where to start calculating EMA energy
 * @param[in] length        length over which to calculate EMA energy
 * @param[in] conf          AEC configuration parameters.
 *
 * @ingroup aec_func
 */
void aec_calc_time_domain_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const aec_config_params_t *conf);

/**
 * @brief Calculate Discrete Fourier Transform (DFT) spectrum of an input time domain vector.
 *
 * This function calculates the spectrum of a real 32bit time domain vector. It calculates an N point real DFT where N is the length of the input vector to output a complex N/2+1 length complex 32bit vector.
 * The N/2+1 complex output values represent spectrum samples from DC up to the Nyquist frequency.
 *
 * The DFT calculation is done in place. After this function call the input and output BFP structures `data` fields point to the same memory.
 * Since DFT is calculated in place, use of the input BFP struct is undefined after this function.
 *
 * @param[out] output    DFT output BFP structure
 * @param[in] input     DFT input BFP structure
 *
 * To allow for inplace transform from N real 32bit values to N/2+1 complex 32bit values, the input vector should have 2 extra real 32bit samples worth of memory.
 * This means that `input->data` should point to a buffer of length `input->length`+2
 *
 * After this function `input->data` and `output->data` point to the same memory address.
 *
 * @ingroup aec_func
 */
void aec_forward_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input);

/**
 * @brief Calculate inverse Discrete Fourier Transform (DFT) of an input spectrum
 *
 * This function calculates a N point inverse real DFT of a complex 32bit where N is 2*(length-1) where length is the length of the input vector.
 * The output is a real 32bit vector of length N.
 *
 * The inverse DFT calculation is done in place. After this operation the input and the output BFP structures `data` fields point to the same memory.
 * Since the calculation is done in place, use of input BFP struct after this function is undefined.
 *
 *  @param[out] output inverse DFT output BFP structure
 *  @param[in] input inverse DFT input BFP structure
 *
 *  After this function `input->data` and `output->data` point to the same memory address.
 *
 * @ingroup aec_func
 */
void aec_inverse_fft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        );

/**
 * @brief Calculate total energy of the X FIFO
 *
 * `X FIFO` is a FIFO of the most recent `X` frames, where `X` is spectrum of one frame of reference input.
 * There's a common X FIFO that is shared between main and shadow filters. It holds `num_main_filter_phases` most recent X frames and the shadow filter uses `num_shadow_filter_phases` most recent frames out of it.
 *
 * This function calculates the energy per X sample index summed across the X FIFO phases.
 * This function also calculates the maximum energy across all samples indices of the output energy vector 
 * 
 * @param[inout] state  AEC state. state->X_energy[ch] and state->max_X_energy[ch] are updated 
 * @param[in]    ch     channel index for which energy calculations are done 
 * @param[in]    recalc_bin The sample index for which energy is recalculated to eliminate quantisation errors
 * 
 * @note
 * @parblock
 * This function implements some speed optimisations which introduce quantisation error. To stop quantisation error build up, in every call of this function, energy for one sample index, which is specified in the `recalc_bin` argument, is recalculated without the optimisations. There are a total of AEC_FD_FRAME_LENGTH samples in the energy vector, so recalc_bin keeps cycling through indexes 0 to AEC_PROC_FRAME_LENGTH/2.
 * @endparblock
 *
 * @ingroup aec_func
 */
void aec_calc_X_fifo_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned recalc_bin);

/**
 * @brief Update X FIFO with the current X frame
 *
 * This function updates the X FIFO by removing the oldest X frame from it and adding the current X frame to it. 
 * This function also calculates sigmaXX which is the exponential moving average of the current X frame energy
 *
 * @param[inout] state AEC state structure. state->shared_state->X_fifo[ch] and state->shared_state->sigma_XX[ch] are updated.
 * @param[in] ch X channel index for which to update X FIFO
 *
 * @ingroup aec_func
 */
void aec_update_X_fifo_and_calc_sigmaXX(
        aec_state_t *state,
        unsigned ch);

/**
 * @brief Calculate error spectrum and estimated mic signal spectrum 
 *
 * This function calculates the error spectrum (`Error`) and estimated mic input spectrum (`Y_hat`)
 * `Y_hat` is calculated as the sum of all phases of the adaptive filter multiplied by the respective phases of the reference input spectrum.
 * Error is calculated by subtracting `Y_hat` from the mic input spectrum `Y`
 *
 * @param[inout] state AEC state structure. state->Error[ch] and state->Y_hat[ch] are updated
 * @param[in] ch mic channel index for which to compute Error and Y_hat
 *
 * @ingroup aec_func
 */
void aec_calc_Error_and_Y_hat(
        aec_state_t *state,
        unsigned ch);

/**
 * @brief Calculate coherence
 *
 * This function calculates the average coherence between mic input signal (`y`) and estimated mic signal (`y_hat`).
 * A metric is calculated using `y` and `y_hat` and the moving average (`coh`) and a slow moving average (`coh_slow`) of that metric is calculated.
 * The coherence values are used to distinguish between situations when filter adaption should continue or freeze and update mu accordingly.
 *
 * @param[inout] state AEC state structure. `state->shared_state->coh_mu_state[ch].coh` and `state->shared_state->coh_mu_state[ch].coh_slow` are updated
 * @param[in] ch mic channel index for which to calculate average coherence
 *
 * @ingroup aec_func
 */
void aec_calc_coherence(
        aec_state_t *state,
        unsigned ch);

/**
 * @brief Calculate AEC filter output signal
 *
 * This function is responsible for windowing the filter `error` signal and creating AEC filter output that can be propagated to downstream stages.
 * `output` is calculated by overlapping and adding current frame's windowed error signal with the previous frame windowed error. This is done to smooth discontinuities in the output as the filter adapts.
 *
 * @param[inout] state AEC state structure. `state->error[ch]`
 * @param[out] output pointer to the output buffer
 * @param[in] ch mic channel index for which to calculate output
 *
 * @ingroup aec_func
 *
 */
void aec_calc_output(
        aec_state_t *state,
        int32_t (*output)[AEC_FRAME_ADVANCE],
        unsigned ch);

/**
 * @brief Calculate normalisation spectrum
 *
 * This function calculates the normalisation spectrum of the reference input signal. This normalised spectrum is later used during filter adaption to scale the adaption to the size of the input signal.
 * The normalisation spectrum is calculated as a time and frequency smoothed energy of the reference input spectrum.
 *
 * The normalisation spectrum is calculated differently for main and shadow filter, so a flag indicating whether this calculation is being done for the main or shadow filter is passed as an input to the function
 *
 * @param[inout] state AEC state structure. state->inv_X_energy[ch] is updated
 * @param[in] ch reference channel index for which to calculate normalisation spectrum
 * @param[in] is_shadow flag indicating filter type. 0: Main filter, 1: Shadow filter
 *
 * @ingroup aec_func
 */
void aec_calc_normalisation_spectrum(
        aec_state_t *state,
        unsigned ch,
        unsigned is_shadow);

/**
 * @brief Compare and update filters. Calculate the adaption step size mu.
 *
 * This function has 2 responsibilities. 
 * First, it compares the energies in the error spectrums of the main and shadow filter with each other and with the mic input spectrum energy, and makes an estimate of how well the filters are performing. Based on this, it optionally modifies the filters by either resetting the filter coefficients or copying one filter into another.
 * Second, it uses the coherence values calculated in aec_calc_coherence as well as information from filter comparison done in step 1 to calculate the adaption step size mu.
 *
 * @param[inout] main_state AEC state structure for the main filter
 * @param[inout] shadow_state AEC state structure for the shadow filter
 *
 * @ingroup aec_func
 */
void aec_compare_filters_and_calc_mu(
        aec_state_t *main_state,
        aec_state_t *shadow_state);

/**
 * @brief Calculate the parameter `T`
 *
 * This function calculates a parameter referred to as `T` that is later used to scale the reference input spectrum in the filter update step.
 * `T` is a function of the adaption step size `mu`, normalisation spectrum `inv_X_energy` and the filter error spectrum `Error`.
 * 
 * @param[inout] state AEC state structure. `state->T[x_ch]` is updated
 * @param[in] y_ch mic channel index
 * @param[in] x_ch reference channel index
 *
 * @ingroup aec_func
 */
void aec_calc_T(
        aec_state_t *state,
        unsigned y_ch,
        unsigned x_ch);

/** @brief Update filter
 *
 * This function updates the adaptive filter spectrum (`H_hat'). It calculates the delta update that is applied to the filter by scaling the X FIFO with the T values computed in `aec_compute_T()` and applies the delta update to `H_hat`.
 * A gradient constraint FFT is then applied to constrain the length of each phase of the filter to avoid wrapping when calculating `y_hat`
 *
 * @param[inout] state AEC state structure. `state->H_hat[y_ch]` is updated
 * @param[in] y_ch mic channel index
 *
 * @ingroup aec_func
 *
 */
void aec_filter_adapt(
        aec_state_t *state,
        unsigned y_ch);

/** @brief Update the X FIFO alternate BFP structure
 *
 * The X FIFO BFP structure is maintained in 2 forms - as a 2 dimensional [x_channels][num_phases] and as a [x_channels * num_phases] 1 dimensional array.
 * This is done in order to optimally access the X FIFO as needed in different functions.
 * After the X FIFO is updated with the current X frame, this function is called in order to copy the 2 dimensional BFP structure into it's 1 dimensional counterpart.
 *
 * @param[inout] state AEC state structure. `state->X_fifo_1d` is updated
 *
 * @ingroup aec_func
 *
 */
void aec_update_X_fifo_1d(
        aec_state_t *state);

/** @brief Calculate a correlation metric between the microphone input and estimated microphone signal
 *
 * This function calculates a metric of resemblance between the mic input and the estimated mic signal. The correlation
 * metric, along with reference signal energy is used to infer presence of near and far end signals in the AEC mic
 * input.
 *
 * @param[in] state AEC state structure. `state->y` and `state->y_hat` are used to calculate the correlation metric
 * @param[in] ch mic channel index for which to calculate the metric
 * @returns correlation metric in float_s32_t format
 *
 * @ingroup aec_func
 *
 */
float_s32_t aec_calc_corr_factor(
        aec_state_t *state,
        unsigned ch);

/** @brief Calculate the energy of the input signal 
 *
 * This function calculates the sum of the energy across all samples of the time domain input channel and
 * returns the maximum energy across all channels.
 *
 * @param[in] input_data Pointer to the input data buffer. The input is assumed to be in Q1.31 fixed point format.
 * @param[in] num_channels Number of input channels.
 * @returns Maximum energy in float_s32_t format.
 *
 * @ingroup aec_func
 *
 */
float_s32_t aec_calc_max_input_energy(
        const int32_t (*input_data)[AEC_FRAME_ADVANCE],
        int num_channels);

/** @brief Reset parts of aec state structure.
 *
 * This function resets parts of AEC state so that the echo canceller starts adapting from a zero filter.
 * 
 * @param[in] pointer to AEC main filter state structure.
 * @param[in] pointer to AEC shadow filter state structure
 * 
 * @ingroup aec_func
 */
void aec_reset_state(aec_state_t *main_state, aec_state_t *shadow_state);

/** @brief Detect activity on input channels.
 * 
 * This function implements a quick check for detecting activity on the input channels. It detects signal presence by checking
 * if the maximum sample in the time domain input frame is above a given threshold.
 *
 * @param[in] input_data Pointer to input data frame. Input is assumed to be in Q1.31 fixed point format.
 * @param[in] active_threshold Threshold for detecting signal activity
 * @param[in] num_channels Number of input data channels
 * @returns 0 if no signal activity on the input channels, 1 if activity detected on the input channels
 * 
 * @ingroup aec_func
 */
uint32_t aec_detect_input_activity(const int32_t (*input_data)[AEC_FRAME_ADVANCE], float_s32_t active_threshold, int32_t num_channels);

//TODO pending documentation and examples for L2 APIs
/**
 * @brief Calculate Error and Y_hat for a channel over a range of bins.
 *
 * @ingroup aec_low_level_func
 */
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

/**
 * @brief Adapt one phase of the adaptive filter
 *
 * @ingroup aec_low_level_func
 */
void aec_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        );

/**
 * @brief Unify bfp_complex_s32_t chunks into a single exponent and headroom
 *
 * @ingroup aec_low_level_func
 */
void aec_l2_bfp_complex_s32_unify_exponent(
        bfp_complex_s32_t *chunks,
        int32_t *final_exp,
        uint32_t *final_hr,
        const uint32_t *mapping,
        uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom);

/**
 * @brief Unify bfp_s32_t chunks into a single exponent and headroom
 *
 * @ingroup aec_low_level_func
 */
void aec_l2_bfp_s32_unify_exponent(
        bfp_s32_t *chunks,
        int32_t *final_exp,
        uint32_t *final_hr,
        const uint32_t *mapping,
        uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom);
#endif
