// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_STATE_H
#define AEC_STATE_H

#include <stdio.h>
#include <string.h>
#include "aec_defines.h"
#include "xmath/xmath.h"


/**
 * @page page_aec_state_h aec_state.h
 * 
 * This header contains definitions for data structures and enums used in lib_aec.
 *
 * @ingroup aec_header_file
 */

/**
 * @defgroup aec_types   AEC Data Structure and Enum Definitions
 */ 

/**
 * @ingroup aec_types
 */
typedef enum {
    AEC_ADAPTION_AUTO, ///< Compute filter adaption config every frame
    AEC_ADAPTION_FORCE_ON, ///< Filter adaption always ON
    AEC_ADAPTION_FORCE_OFF, ///< Filter adaption always OFF
} aec_adaption_e;

/**
 * @ingroup aec_types
 */
typedef enum {
    LOW_REF = -4,    ///< Not much reference so no point in acting on AEC filter logic
    ERROR = -3,      ///< something has gone wrong, zero shadow filter
    ZERO = -2,       ///< shadow filter has been reset multiple times, zero shadow filter
    RESET = -1,      ///< copy main filter to shadow filter
    EQUAL = 0,       ///< main filter and shadow filter are similar
    SIGMA = 1,       ///< shadow filter bit better than main, reset sigma_xx for faster convergence
    COPY = 2,        ///< shadow filter much better, copy to main
}shadow_state_e;

/**
 * @ingroup aec_types
 */
typedef struct {
    /** Update rate of `coh`.*/
    float_s32_t coh_alpha;
    /** Update rate of `coh_slow`.*/
    float_s32_t coh_slow_alpha;
    /** Adaption frozen if coh below (coh_thresh_slow*coh_slow)*/
    float_s32_t coh_thresh_slow;
    /** Adaption frozen if coh below coh_thresh_abs.*/
    float_s32_t coh_thresh_abs;
    /** Scalefactor for scaling the calculated mu.*/
    float_s32_t mu_scalar;
    /** Parameter to avoid divide by 0 in coh calculation.*/
    float_s32_t eps;
    /** -20dB threshold*/
    float_s32_t thresh_minus20dB;
    /** X_energy threshold used for determining if the signal has enough reference energy for sensible coherence mu calculation*/ 
    float_s32_t x_energy_thresh;
    /** Number of frames after low coherence, adaption frozen for.*/
    unsigned mu_coh_time;
    /** Number of frames after shadow filter use, the adaption is fast for*/
    unsigned mu_shad_time;
    /** Filter adaption mode. Auto, force ON or force OFF*/
    aec_adaption_e adaption_config;
    /** Fixed mu value used when filter adaption is forced ON*/
    int32_t force_adaption_mu_q30;
} coherence_mu_config_params_t;

/**
 * @ingroup aec_types
 */
typedef struct {
    /** threshold for resetting sigma_XX.*/
    float_s32_t shadow_sigma_thresh;
    /** threshold for copying shadow filter.*/
    float_s32_t shadow_copy_thresh;
    /** threshold for resetting shadow filter.*/
    float_s32_t shadow_reset_thresh;
    /** threshold for turning off shadow filter reset if reference delay is large*/
    float_s32_t shadow_delay_thresh;
    /** X energy threshold used for deciding whether the system has enough reference energy for main and shadow filter
     * comparison to make sense*/
    float_s32_t x_energy_thresh;
    /** fixed mu value used during shadow filter adaption.*/
    float_s32_t shadow_mu;
    /** Number of times shadow filter needs to be better before it gets copied to main filter.*/
    int32_t shadow_better_thresh;
    /** Number of times shadow filter is reset by copying the main filter to it before it gets zeroed.*/
    int32_t shadow_zero_thresh;
    /** Number of frames between zeroing resets of shadow filter.*/
    int32_t shadow_reset_timer;
}shadow_filt_config_params_t;

/**
 * @ingroup aec_types
 */
typedef struct {
    /** bypass AEC flag.*/
    int bypass;
    /** parameter for deriving the gamma value that used in normalisation spectrum calculation. gamma is calculated as
     * 2^gamma_log2*/
    int gamma_log2;
    /** parameter used for deriving the alpha value used while calculating EMA of X_energy to calculate sigma_XX.*/
    uint32_t sigma_xx_shift;
    /** delta value used in normalisation spectrum computation when adaption is forced as always ON.*/
    float_s32_t delta_adaption_force_on;
    /** Lower limit of delta computed using fractional regularisation.*/
    float_s32_t delta_min;
    /** coefficient index used to track H_hat index when sending H_hat values over the host control interface.*/
    uint32_t coeff_index;
    /** alpha used while calculating y_ema_energy, x_ema_energy and error_ema_energy.*/
    uq2_30 ema_alpha_q30;
}aec_core_config_params_t;

/**
 * @brief AEC control parameters.
 *
 * This structure contains control parameters that the user can modify at run time.
 * @ingroup aec_types
 */
typedef struct {
    /** Coherence mu related control params.*/
    coherence_mu_config_params_t coh_mu_conf;
    /** Shadow filter related control params.*/
    shadow_filt_config_params_t shadow_filt_conf;
    /** All AEC control params except those for coherence mu and shadow filter.*/
    aec_core_config_params_t aec_core_conf;
}aec_config_params_t;

/**
 * @ingroup aec_types
 */
typedef struct {
    float_s32_t coh; ///< Moving average coherence
    float_s32_t coh_slow; ///< Slow moving average coherence

    int32_t mu_coh_count; ///< Counter for tracking number of frames coherence has been low for.
    int32_t mu_shad_count; ///< Counter for tracking number of frames shadow filter has been used in
    float_s32_t coh_mu[AEC_LIB_MAX_X_CHANNELS]; ///< Coherence mu
}coherence_mu_params_t;


/**
 * @ingroup aec_types
 */
typedef struct {
    int32_t shadow_flag[AEC_LIB_MAX_Y_CHANNELS]; ///< shadow_state_e enum indicating shadow filter status
    int shadow_reset_count[AEC_LIB_MAX_Y_CHANNELS]; ///< counter for tracking shadow filter resets
    int shadow_better_count[AEC_LIB_MAX_Y_CHANNELS]; ///< counter for tracking shadow filter copy to main filter
}shadow_filter_params_t;

/**
 * @brief AEC shared state structure.
 *
 * Data structures holding AEC persistent state that is common between main filter and shadow filter.
 * aec_state_t::shared_state for both main and shadow filter point to the common aec_shared_t structure.
 *
 * @ingroup aec_types
 */
 //! [aec_shared_state_t]
typedef struct {
    /** BFP array pointing to the reference input spectrum phases. The term \b phase refers to the spectrum data for a
     * frame. Multiple phases means multiple frames of data.
     *
     * For example, 10 phases would mean the 10 most recent frames of data.
     * Each phase spectrum, pointed to by X_fifo[i][j]->data is stored as a length AEC_FD_FRAME_LENGTH, complex 32bit
     * array.
     *
     * The phases are ordered from most recent to least recent in the X_fifo. For example, for an AEC configuration of 2
     * x-channels and 10 phases per x channel, 10 frames of X data spectrum is stored in the X_fifo. For a given x
     * channel, say x channel 0, X_fifo[0][0] points to the most recent frame's X spectrum and X_fifo[0][9] points to
     * the last phase, i.e the least recent frame's X spectrum.*/
    bfp_complex_s32_t X_fifo[AEC_LIB_MAX_X_CHANNELS][AEC_LIB_MAX_PHASES];

    /** BFP array pointing to reference input signal spectrum. The X data values are stored as a length
     * AEC_FD_FRAME_LENGTH complex 32bit array per x channel.*/
    bfp_complex_s32_t X[AEC_LIB_MAX_X_CHANNELS];

    /** BFP array pointing to mic input signal spectrum. The Y data values are stored as a length
     * AEC_FD_FRAME_LENGTH complex 32bit array per y channel.*/
    bfp_complex_s32_t Y[AEC_LIB_MAX_Y_CHANNELS];
    
    /** BFP array pointing to time domain mic input processing block. The y data values are stored as length
     * AEC_PROC_FRAME_LENGTH, 32bit integer array per y channel.*/
    bfp_s32_t y[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to time domain reference input processing block. The x data values are stored as length
     * AEC_PROC_FRAME_LENGTH, 32bit integer array per x channel.*/
    bfp_s32_t x[AEC_LIB_MAX_X_CHANNELS];

    /** BFP array pointing to time domain mic input values from the previous frame. These are put together with the new
     * samples received in the current frame to make a AEC_PROC_FRAME_LENGTH processing block. The prev_y data values
     * are stored as length (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 32bit integer array per y channel.*/
    bfp_s32_t prev_y[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to time domain reference input values from the previous frame. These are put together with
     * the new samples received in the current frame to make a AEC_PROC_FRAME_LENGTH processing block. The prev_x data
     * values are stored as length (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE), 32bit integer array per x channel.*/
    bfp_s32_t prev_x[AEC_LIB_MAX_X_CHANNELS];
    
    /** BFP array pointing to sigma_XX values which are the weighted average of the X_energy signal. The sigma_XX data
     * is stored as 32bit integer array of length AEC_FD_FRAME_LENGTH*/
    bfp_s32_t sigma_XX[AEC_LIB_MAX_X_CHANNELS];

    /** Exponential moving average of the time domain mic signal energy. This is calculated by calculating energy
     * per sample and summing across all samples. Stored in a y channels array
     * with every value stored as a 32bit integer mantissa and exponent.*/
    float_s32_t y_ema_energy[AEC_LIB_MAX_Y_CHANNELS];

    /** Exponential moving average of the time domain reference signal energy. This is calculated by calculating energy
     * per sample and summing across all samples. Stored in a x channels array with every value stored as a 32bit
     * integer mantissa and exponent.*/
    float_s32_t x_ema_energy[AEC_LIB_MAX_X_CHANNELS];

    /** Energy of the mic input spectrum. This is calculated by calculating the energy per bin and summing across all
     * bins. Stored in a y channels array with every value stored as a 32bit integer mantissa and exponent.*/
    float_s32_t overall_Y[AEC_LIB_MAX_Y_CHANNELS];

    /** Sum of the X_energy across all bins for a given x channel. Stored in a x channels array with every value stored
     * as a 32bit integer mantissa and exponent.*/ 
    float_s32_t sum_X_energy[AEC_LIB_MAX_X_CHANNELS]; 
    
    /** Structure containing coherence mu calculation related parameters.*/
    coherence_mu_params_t coh_mu_state[AEC_LIB_MAX_Y_CHANNELS];

    /** Structure containing shadow filter related parameters.*/
    shadow_filter_params_t shadow_filter_params;

    /** Structure containing AEC control parameters. These are initialised to the default values and can be changed at
     * runtime by the user.*/
    aec_config_params_t config_params;

    /** Number of mic input channels that the AEC is configured for. This is the input parameter num_y_channels that
     * aec_init() gets called with.*/
    unsigned num_y_channels;

    /** Number of reference input channels that the AEC is configured for. This is the input parameter num_x_channels that
     * aec_init() gets called with.*/
    unsigned num_x_channels;
}aec_shared_state_t;
//! [aec_shared_state_t]

/**
 * @brief AEC state structure.
 *
 * Data structures holding AEC persistent state. There are 2 instances of aec_state_t maintained within AEC; one for
 * main filter and one for shadow filter specific state.
 *
 * @ingroup aec_types
 */
//! [aec_state_t]
typedef struct {
    /** BFP array pointing to estimated mic signal spectrum. The Y_data data values are stored as length
     * AEC_FD_FRAME_LENGTH, complex 32bit array per y channel.*/
    bfp_complex_s32_t Y_hat[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to adaptive filter error signal spectrum. The Error data is stored as length
     * AEC_FD_FRAME_LENGTH, complex 32bit array per y channel.*/
    bfp_complex_s32_t Error[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to the adaptive filter spectrum.
     * The filter spectrum is stored as a num_y_channels x total_phases_across_all_x_channels array where each H_hat[i][j]
     * entry points to the spectrum of a single phase.
     *
     * Number of phases in the filter refers to its tail length. A filter with more phases would be able to model a longer
     * echo thereby causing better echo cancellation.
     *
     * For example, for a 2 y-channels, 3 x-channels, 10 phases per x channel configuration,
     * the filter spectrum phases are stored in a 2x30 array. For a given y channel, say y channel 0, H_hat[0][0] to
     * H_hat[0][9] points to 10 phases of H_hat<SUB>y0x0</SUB>, H_hat[0][10] to H_hat[0][19] points to 10 phases of
     * H_hat<SUB>y0x1</SUB> and H_hat[0][20] to H_hat[0][29] points to 10 phases of H_hat<SUB>y0x2</SUB>.
     *
     * Each filter phase data which is pointed to by H_hat[i][j].data is stored as AEC_FD_FRAME_LENGTH complex 32bit
     * array.*/
    bfp_complex_s32_t H_hat[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_PHASES];

    /** BFP array pointing to all phases of reference input spectrum across all x channels. Here, the reference input
     * spectrum is saved in a 1 dimensional array of phases, with x channel 0 phases followed by x channel 1 phases and
     * so on.  For example, for a 2 x-channels, 10 phases per x channel configuration, X_fifo_1d[0] to X_fifo_1d[9]
     * points to the 10 phases for channel 0 and X_fifo[10] to X_fifo[19] points to the 10 phases for channel 1.
     *
     * Each X data spectrum phase pointed to by X_fifo_1d[i][j].data is stored as length AEC_FD_FRAME_LENGTH complex
     * 32bit array.*/
    bfp_complex_s32_t X_fifo_1d[AEC_LIB_MAX_PHASES];

    /** BFP array pointing to T values which are stored as a length AEC_FD_FRAME_LENGTH, complex array per x channel.*/ 
    bfp_complex_s32_t T[AEC_LIB_MAX_X_CHANNELS]; 

    /** BFP array pointing to the normalisation spectrum which are stored as a length AEC_FD_FRAME_LENGTH, 32bit
     * integer array per x channel.*/ 
    bfp_s32_t inv_X_energy[AEC_LIB_MAX_X_CHANNELS];

    /** BFP array pointing to the X_energy data which is the energy per bin of the X spectrum summed over all phases of
     * the X data. X_energy data is stored as a length AEC_FD_FRAME_LENGTH, integer 32bit array per x channel.*/
    bfp_s32_t X_energy[AEC_LIB_MAX_X_CHANNELS];

    /** BFP array pointing to time domain overlap data values which are used in the overlap add operation done while
     * calculating the echo canceller time domain output. Stored as a length 32, 32 bit integer array per y channel.*/
    bfp_s32_t overlap[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to the time domain estimated mic signal. Stored as length AEC_PROC_FRAME_LENGTH, 32 bit
     * integer array per y channel.*/ 
    bfp_s32_t y_hat[AEC_LIB_MAX_Y_CHANNELS];

    /** BFP array pointing to the time domain adaptive filter error signal. Stored as length AEC_PROC_FRAME_LENGTH, 32 bit
     * integer array per y channel.*/ 
    bfp_s32_t error[AEC_LIB_MAX_Y_CHANNELS];

    /** mu values for every x-y pair stored as 32 bit integer mantissa and 32 bit integer exponent*/
    float_s32_t mu[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_X_CHANNELS];

    /** Exponential moving average of the time domain adaptive filter error signal energy. Stored in an x channels array
     * with every value stored as a 32bit integer mantissa and exponent.*/
    float_s32_t error_ema_energy[AEC_LIB_MAX_Y_CHANNELS];

    /** Energy of the adaptive filter error spectrum. Stored in a y channels array with every value stored as a 32bit
     * integer mantissa and exponent.*/
    float_s32_t overall_Error[AEC_LIB_MAX_Y_CHANNELS];

    /** Maximum X energy across all values of X_energy for a given x channel. Stored in an x channels array with every
     * value stored as a 32bit integer mantissa and exponent.*/
    float_s32_t max_X_energy[AEC_LIB_MAX_X_CHANNELS];
    
    /** fractional regularisation scalefactor.*/
    float_s32_t delta_scale;

    /** delta parameter used in the normalisation spectrum calculation.*/
    float_s32_t delta; 
    
    /** pointer to the state data shared between main and shadow filter.*/
    aec_shared_state_t *shared_state;
    
    /** Number of filter phases per x-y pair that AEC filter is configured for. This is the input argument
     * num_main_filter_phases or num_shadow_filter_phases, depending on which filter the aec_state_t is instantiated
     * for, passed in aec_init() call.*/
    unsigned num_phases; 
}aec_state_t;
//! [aec_state_t]

#endif
