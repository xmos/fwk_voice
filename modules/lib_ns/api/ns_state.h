// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef NS_STATE_H
#define NS_STATE_H

#include "xmath/xmath.h"

/**
 * @page page_ns_state_h ns_state.h 
 * 
 * This header contains definitions for data structure and defines.
 * 
 * This header is automatically included by `ns_api.h`
 */

/**
 * @defgroup ns_defs   NS API structure definitions
 */

/**
 * @brief Length of the frame of data on which the NS will operate.
 *
 * @ingroup ns_defs
 */
#define NS_FRAME_ADVANCE           (240)

/** Time domain samples block length used internally.
 *
 * @ingroup ns_defs
 */
#define NS_PROC_FRAME_LENGTH (512)

/** Number of bins of spectrum data computed when doing a DFT of a NS_PROC_FRAME_LENGTH length time domain vector. The
 * NS_PROC_FRAME_BINS spectrum values represent the bins from DC to Nyquist.
 *
 * @ingroup ns_defs
 */   
#define NS_PROC_FRAME_BINS        ((NS_PROC_FRAME_LENGTH / 2) + 1)

/** The exponent used internally to keep q1.31 format.
 *
 * @ingroup ns_defs
 */
#define NS_INT_EXP (-31)

/** The length of the window applied in time domain
 * 
 * @ingroup ns_defs
 */
#define NS_WINDOW_LENGTH (480)

/** 
 * @brief NS state structure
 * 
 * This structure holds the current state of the NS instance and members are updated each
 * time that `ns_process_frame()` runs. Many of these members are exponentially-weighted
 * moving averages (EWMA) which influence the behaviour of the NS filter. 
 * The user should not directly modify any of these members. 
 * 
 * @ingroup ns_defs
 */
typedef struct {

    //Dynamic MCRA filter coefficients
    /** BFP structure to hold the local energy. */
    bfp_s32_t S;
    /** BFP structure to hold the minimum local energy within 10 frames. */
    bfp_s32_t S_min;
    /** BFP structure to hold the temporary local energy. */
    bfp_s32_t S_tmp;
    /** BFP structure to hold the conditional signal presence probability*/
    bfp_s32_t p;
    /** BFP structure to hold the time-varying smoothing parameter. */
    bfp_s32_t alpha_d_tilde;
    /** BFP structure to hold the noise estimation. */
    bfp_s32_t lambda_hat;

    /** int32_t array to hold the data for S. */
    int32_t data_S [NS_PROC_FRAME_BINS];
    /** int32_t array to hold the data for S_min. */
    int32_t data_S_min [NS_PROC_FRAME_BINS];
    /** int32_t array to hold the data for S_tmp. */
    int32_t data_S_tmp [NS_PROC_FRAME_BINS];
    /** int32_t array to hold the data for p. */
    int32_t data_p [NS_PROC_FRAME_BINS];
    /** int32_t array to hold the data for alpha_d_tilde. */
    int32_t data_adt [NS_PROC_FRAME_BINS];
    /** int32_t array to hold the data for lambda_hat. */
    int32_t data_lambda_hat [NS_PROC_FRAME_BINS];

    //Data needed for the frame packing and windowing
    /** BFP structure to hold the previous frame. */
    bfp_s32_t prev_frame;
    /** BFP structure to hold the overlap. */
    bfp_s32_t overlap;
    /** BFP structure to hold the first part of the window. */
    bfp_s32_t wind;
    /** BFP structure to hold the second part of the window. */
    bfp_s32_t rev_wind;

    /** int32_t array to hold the data for prev_frame. */
    int32_t data_prev_frame [NS_PROC_FRAME_LENGTH - NS_FRAME_ADVANCE];
    /** int32_t array to hold the data for overlap. */
    int32_t data_ovelap [NS_FRAME_ADVANCE];
    /** int32_t array to hold the data for rev_wind. */
    int32_t data_rev_wind [NS_WINDOW_LENGTH / 2];

    //Static MCRA filter coefficients
    //If modifying any parameters, modify one_minus parameters as well!
    /** EWMA of the energy ratio to calculate p. */
    float_s32_t delta;
    /** EWMA of the smoothing parameter for alpha_d_tilde. */
    float_s32_t alpha_d;
    /** EWMA of the smoothing parameter for S. */
    float_s32_t alpha_s;
    /** EWMA of the smoothing parameter for p. */
    float_s32_t alpha_p;
    /** EWMA of the 1 - alpha_d parameter. */
    float_s32_t one_minus_aplha_d;
    /** EWMA of the 1 - alpha_s parameter. */
    float_s32_t one_minus_alpha_s;
    /** EWMA of the 1 - alpha_p parameter. */
    float_s32_t one_minus_alpha_p;

    /** Filter reset period value for auto-reset. */
    unsigned reset_period;
    /** Filter reset counter. */
    unsigned reset_counter;

} ns_state_t;

#endif
