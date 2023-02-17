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
 * @ingroup ic_state
 */
typedef enum {
    IC_ADAPTION_AUTO  = 0,
    IC_ADAPTION_FORCE_ON = 1,
    IC_ADAPTION_FORCE_OFF = 2
} adaption_config_e;

/**
 * @ingroup ic_state
 */
typedef enum {HOLD = 2, 
            ADAPT = 1, 
            ADAPT_SLOW = 0, 
            UNSTABLE = -1, 
            FORCE_ADAPT = -2, 
            FORCE_HOLD = -3
}control_flag_e;

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
    q2_30 ema_alpha_q30;
    /** Delta value used in denominator to avoid large values when calculating inverse
     * X energy. */
    float_s32_t delta;
}ic_config_params_t;

/**
 * @brief IC adaption controller configuration structure
 *
 * This structure contains configuration settings that can be changed to alter the
 * behaviour of the adaption controller. This includes processing of the raw
 * VNR probability input and optional stability controller logic.
 * It is automatically included as part of the IC state and initialised by ic_init().
 * 
 * The initial values for these configuration parameters are defined in ic_defines.h.
 * 
 * @ingroup ic_state
 */
typedef struct {

    /** Alpha for EMA input/output energy calculation. */
    q2_30 energy_alpha_q30;

    /** Fast ratio threshold to detect instability. */
    float_s32_t fast_ratio_threshold;
    /** Setting of H_hat leakage which gets set if vnr detects high voice probability. */
    float_s32_t high_input_vnr_hold_leakage_alpha;
    /** Setting of H_hat leakage which gets set if fast ratio exceeds a threshold. */
    float_s32_t instability_recovery_leakage_alpha;

    /** VNR input threshold which decides whether to hold or adapt the filter. */
    float_s32_t input_vnr_threshold;
    /** VNR high threshold to leak the filter is the speech level is high. */
    float_s32_t input_vnr_threshold_high;
    /** VNR low threshold to adapt faster when the speech level is low. */
    float_s32_t input_vnr_threshold_low;

    /** Limits number of frames for which mu and leakage_alpha could be adapted. */
    uint32_t adapt_counter_limit;

    /** Boolean which controls whether the IC adapts when ic_adapt() is called. */
    uint8_t enable_adaption;
    /** Enum which controls the way mu and leakage_alpha are being adjusted. */
    adaption_config_e adaption_config;

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

    /** EMWA of input frame energy. */
    float_s32_t input_energy;

    /** EMWA of output frame energy. */
    float_s32_t output_energy;

    /** Ratio between output and input EMWA energies. */
    float_s32_t fast_ratio;

    /** Adaption counter which counts number of frames has been adapted. */
    uint32_t adapt_counter;

    /** Flag that represents the state of the filter. */
    control_flag_e control_flag;

    /** Configuration parameters for the adaption controller. */
    ic_adaption_controller_config_t adaption_controller_config;
} ic_adaption_controller_state_t;

// Struct to keep VNR predictions and the EMA alpha
typedef struct {
    vnr_feature_state_t feature_state[2];
    float_s32_t input_vnr_pred;
    float_s32_t output_vnr_pred;
    q2_30 pred_alpha_q30;
}vnr_pred_state_t;

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
    // Copy of first 240 y prev samples before they get updated in ic_frame_init(). This, along with the updated y_prev_samples is used to get back the 512 samples of the input processing frame when it's needed again in ic_reset_filter(). All of this is needed since due to in-place DFT processing, y_bfp memory would hold freq domain at the point of ic_reset_filter() call*/  
    int32_t DWORD_ALIGNED y_prev_samples_copy[IC_Y_CHANNELS][IC_FRAME_ADVANCE];  
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
    complex_s32_t DWORD_ALIGNED Error[IC_Y_CHANNELS][IC_FD_FRAME_LENGTH];

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
    /** Alpha used for leaking away H_hat, allowing filter to slowly forget adaption. */
    float_s32_t leakage_alpha;
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
    /** Input and Output VNR Prediction related state*/
    vnr_pred_state_t vnr_pred_state;
} ic_state_t;

#endif
