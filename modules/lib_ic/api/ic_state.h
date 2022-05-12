// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_STATE_H
#define IC_STATE_H

#include "ic_defines.h"


/**
 * @page page_ic_state_h ic_state.h
 * 
 * This header contains definitions for data structures used in lib_ic.
 * It also contains the configuration sub-structures which control the operation
 * of the interference canceller during run-time.
 *
 * @ingroup ic_header_file
 */

/**
 * @defgroup ic_state   IC Data Structures
 */ 


/**
 * @brief IC configuration structure
 *
 * This structure contains configuration settings that can be changed to alter the
 * behaviour of the IC instance. An instance of this structure is is automatically
 * included as part of the IC state.
 * 
 * It controls the behaviour of the main filter and normalisation thereof.
 * The initial values for these configuration parameters are defined in ic_defines.h
 * and are initialised by ic_init().
 *
 * @ingroup ic_state
 */
typedef struct {
    /** Boolean to control bypassing of filter stage and adaption stage. When set 
     * the delayed y audio samples are passed unprocessed to the output.
     * It is recommended to perform an initialisation of the instance after bypass
     * is set as the room transfer function may have changed during that time. */
    uint8_t bypass;
    /** Up scaling factor for X energy calculation used for normalisation. */
    int32_t gamma_log2;
    /** Down scaling factor for X energy for used for normalisation. */
    uint32_t sigma_xx_shift;
    /** Alpha used for calculating error_ema_energy in adapt. */
    fixed_s32_t ema_alpha_q30;
    /** Delta value used in denominator to avoid large values when calculating inverse
     * X energy. */
    float_s32_t delta;
}ic_config_params_t;


/**
 * @brief IC adaption controller configuration structure
 *
 * This structure contains configuration settings that can be changed to alter the
 * behaviour of the adaption controller. This includes processing of the raw
 * VAD probability input and optional stability controller logic.
 * It is automatically included as part of the IC state and initialised by ic_init().
 * 
 * The initial values for these configuration parameters are defined in ic_defines.h.
 * 
 * @ingroup ic_state
 */
typedef struct {
    /** Alpha used for leaking away H_hat, allowing filter to slowly forget adaption. */
    float_s32_t leakage_alpha;
    /** Alpha used for low pass filtering the voice chance estimate based on VAD input. */
    float_s32_t voice_chance_alpha;

    /** Slow alpha used filtering input and output energies of IC. */
    fixed_s32_t energy_alpha_slow_q30;
    /** Fast alpha used filtering input and output energies of IC. */
    fixed_s32_t energy_alpha_fast_q30;

    /** Ratio of the output to input at which the filter will reset.
     * Setting it to 2.0 is a good rule of thumb. */
    float_s32_t out_to_in_ratio_limit;
    /** Setting of H_hat leakage which gets set if fast ratio exceeds a threshold. */
    float_s32_t instability_recovery_leakage_alpha;

    /** Boolean which controls whether the IC adapts when ic_adapt() is called. */
    uint8_t enable_adaption;
    /** Boolean which controls whether Mu is automatically adjusted from the VAD input. */
    uint8_t enable_adaption_controller;
    /** Boolean which controls whether to enable detection and recovery from instability
     * in the case when the adaption controller is enabled. */
    uint8_t enable_filter_instability_recovery;

}ic_adaption_controller_config_t;


/**
 * @brief IC adaption controller state structure
 *
 * This structure contains state used for the instance of the adaption controller
 * logic. It is automatically included as part of the IC state and initialised by
 * ic_init().
 * 
 * @ingroup ic_state
 */
typedef struct {
    /** Post processed VAD value. */
    float_s32_t smoothed_voice_chance;

    /** Slow filtered value of IC input energy. */
    float_s32_t input_energy_slow;
    /** Slow filtered value of IC output energy. */
    float_s32_t output_energy_slow;

    /** Fast filtered value of IC input energy. */
    float_s32_t input_energy_fast;
    /** Fast filtered value of IC output energy. */
    float_s32_t output_energy_fast;

    /** Configuration parameters for the adaption controller. */
    ic_adaption_controller_config_t adaption_controller_config;
} ic_adaption_controller_state_t;


/**
 * @brief IC state structure
 *
 * This is the main state structure for an instance of the Interference Canceller.
 * Before use it must be initialised using the ic_init() function. It contains
 * everything needed for the IC instance including configuration and internal state
 * of both the filter, adaption logic and adaption controller.
 * 
 * @ingroup ic_state
 */
typedef struct {
    /** BFP array pointing to the time domain y input signal. */
    bfp_s32_t y_bfp[IC_Y_CHANNELS];
    /** BFP array pointing to the frequency domain Y input signal. */
    bfp_complex_s32_t Y_bfp[IC_Y_CHANNELS];
    /** Storage for y and Y mantissas. Note FFT is done in-place 
     * so the y storage is reused for Y. */
    int32_t DWORD_ALIGNED y[IC_Y_CHANNELS][IC_FRAME_LENGTH + FFT_PADDING];

    /** BFP array pointing to the time domain x input signal.*/
    bfp_s32_t x_bfp[IC_X_CHANNELS];
    /** BFP array pointing to the frequency domain X input signal.*/
    bfp_complex_s32_t X_bfp[IC_X_CHANNELS];
    /** Storage for x and X mantissas. Note FFT is done in-place 
     * so the x storage is reused for X. */
    int32_t DWORD_ALIGNED x[IC_X_CHANNELS][IC_FRAME_LENGTH + FFT_PADDING];

    /** BFP array pointing to previous y samples which are used for framing. */
    bfp_s32_t prev_y_bfp[IC_Y_CHANNELS];
    /** Storage for previous y mantissas. */
    int32_t DWORD_ALIGNED y_prev_samples[IC_Y_CHANNELS][IC_FRAME_LENGTH - IC_FRAME_ADVANCE]; //272 samples history
    /** BFP array pointing to previous x samples which are used for framing. */
    bfp_s32_t prev_x_bfp[IC_X_CHANNELS];
    /** Storage for previous x mantissas. */
    int32_t DWORD_ALIGNED x_prev_samples[IC_X_CHANNELS][IC_FRAME_LENGTH - IC_FRAME_ADVANCE]; //272 samples history


    /** BFP array pointing to the estimated frequency domain Y signal. */
    bfp_complex_s32_t Y_hat_bfp[IC_Y_CHANNELS];
    /** Storage for Y_hat mantissas. */
    complex_s32_t DWORD_ALIGNED Y_hat[IC_Y_CHANNELS][IC_FD_FRAME_LENGTH];

    /** BFP array pointing to the frequency domain Error output. */
    bfp_complex_s32_t Error_bfp[IC_Y_CHANNELS];
    /** BFP array pointing to the time domain Error output. */
    bfp_s32_t error_bfp[IC_Y_CHANNELS];
    /** Storage for Error and error mantissas. Note IFFT is done in-place 
     * so the Error storage is reused for error. */
    int32_t DWORD_ALIGNED Error[IC_Y_CHANNELS][IC_FRAME_LENGTH + FFT_PADDING];

    /** BFP array pointing to the frequency domain estimate of transfer function. */
    bfp_complex_s32_t H_hat_bfp[IC_Y_CHANNELS][IC_X_CHANNELS*IC_FILTER_PHASES];
    /** Storage for H_hat mantissas. */
    complex_s32_t DWORD_ALIGNED H_hat[IC_Y_CHANNELS][IC_FILTER_PHASES*IC_X_CHANNELS][IC_FD_FRAME_LENGTH];

    /** BFP array pointing to the frequency domain X input history used for calculating normalisation. */
    bfp_complex_s32_t X_fifo_bfp[IC_X_CHANNELS][IC_FILTER_PHASES];
    /** 1D alias of the frequency domain X input history used for calculating normalisation. */
    bfp_complex_s32_t X_fifo_1d_bfp[IC_X_CHANNELS*IC_FILTER_PHASES];
    /** Storage for X_fifo mantissas. */
    complex_s32_t DWORD_ALIGNED X_fifo[IC_X_CHANNELS][IC_FILTER_PHASES][IC_FD_FRAME_LENGTH];

    /** BFP array pointing to the frequency domain T used for adapting the filter coefficients (H). 
     * Note there is no associated storage because we re-use the x input array as a memory optimisation. */
    bfp_complex_s32_t T_bfp[IC_X_CHANNELS];

    /** BFP array pointing to the inverse X energies used for normalisation. */
    bfp_s32_t inv_X_energy_bfp[IC_X_CHANNELS];
    /** Storage for inv_X_energy mantissas. */
    int32_t DWORD_ALIGNED inv_X_energy[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];
    /** BFP array pointing to the X energies. */
    bfp_s32_t X_energy_bfp[IC_X_CHANNELS];
    /** Storage for X_energy mantissas. */
    int32_t DWORD_ALIGNED X_energy[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];
    /** Index state used for calculating energy across all X bins. */
    unsigned X_energy_recalc_bin;

    /** BFP array pointing to the overlap array used for windowing and overlap operations. */
    bfp_s32_t overlap_bfp[IC_Y_CHANNELS];
    /** Storage for overlap mantissas. */
    int32_t DWORD_ALIGNED overlap[IC_Y_CHANNELS][IC_FRAME_OVERLAP];

    /** FIFO for delaying y channel (w.r.t x) to enable adaptive filter to be effective. */
    int32_t y_input_delay[IC_Y_CHANNELS][IC_Y_CHANNEL_DELAY_SAMPS];
    /** Index state used for keeping track of y delay FIFO. */
    uint32_t y_delay_idx[IC_Y_CHANNELS];

    /** Mu value used for controlling adaption rate. */
    float_s32_t mu[IC_Y_CHANNELS][IC_X_CHANNELS];
    /** Filtered error energy. */
    float_s32_t error_ema_energy[IC_Y_CHANNELS];
    /** Used to keep track of peak X energy. */
    float_s32_t max_X_energy[IC_X_CHANNELS]; 

    /** BFP array pointing to the EMA filtered X input energy. */
    bfp_s32_t sigma_XX_bfp[IC_X_CHANNELS];
    /** Storage for sigma_XX mantissas. */
    int32_t DWORD_ALIGNED sigma_XX[IC_X_CHANNELS][IC_FD_FRAME_LENGTH];

    /** X energy sum used for maintaining the X FIFO. */
    float_s32_t sum_X_energy[IC_X_CHANNELS]; 

    /** Configuration parameters for the IC. */
    ic_config_params_t config_params;
    /** State and configuration parameters for the IC adaption controller. */
    ic_adaption_controller_state_t ic_adaption_controller_state;
} ic_state_t;

#endif
