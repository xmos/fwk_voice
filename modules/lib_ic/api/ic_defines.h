// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_DEFINES_H
#define IC_DEFINES_H

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"


/**
 * @page page_ic_defines_h ic_defines.h
 * 
 * This header contains lib_ic public defines that are used to configure
 * the interference canceller when ic_init() is called.
 *
 * @ingroup ic_header_file
 */

/**
 * @defgroup ic_defines   IC #define constants
 */ 

/** Initial MU value applied on startup. MU controls the adaption rate of the IC and 
 * is normally adjusted by the adaption rate controller during operation.
 * @ingroup ic_defines */
#define IC_INIT_MU                                  0.369566 //From Python test_wav_ic
/** Alpha used for calculating y_ema_energy, x_ema_energy and error_ema_energy.
 * @ingroup ic_defines */
#define IC_INIT_EMA_ALPHA                           0.9995117188 //From Python model
/** Alpha used for leaking away H_hat, allowing filter to slowly forget adaption. This
 * value is adjusted by the adaption rate controller if instability is detected.
 * @ingroup ic_defines */
#define IC_INIT_LEAKAGE_ALPHA                       1.0 //From Python model

/** The number of filter phases supported by the IC. Each filter phase represents 15ms of
 * filter length. Hence a 10 phase filter will allow cancellation of noise sources with
 * up to 150ms of echo tail length. There is a tradeoff between adaption speed and 
 * maximum cancellation of the filter; increasing the number of phases will increase
 * the maximum cancellation at the cost of increasesed xCORE resource usage and slower
 * adaption times.
 * @ingroup ic_defines */
#define IC_FILTER_PHASES                            10 //From Python model
/** This is the delay, in samples that one of the microphone signals is delayed in order
 * for the filter to be effective. A larger number increases the delay through the filter
 * but may improve cancellation.
 * The group delay through the IC filter is 32 + this number of samples
 * @ingroup ic_defines */
#define IC_Y_CHANNEL_DELAY_SAMPS                    180 //From Python model

/** Down scaling factor for X energy calculation used for normalisation.
 * @ingroup ic_defines */
#define IC_INIT_SIGMA_XX_SHIFT                      11  //From XC IC and Python AEC
/** Up scaling factor for X energy calculation for used for LMS normalisation.
 * @ingroup ic_defines */
#define IC_INIT_GAMMA_LOG2                          1   //From Python IC (2^1 = 2.0)
/** Delta value used in denominator to avoid large values when calculating inverse X energy.
 * @ingroup ic_defines */
// #define IC_INIT_DELTA                               0.0156249999963620211929 //From Python test_wav_ic
#define IC_INIT_DELTA                               0.00007 //From XC


/** Boolean which controls whether to enable detection and recovery from instability. 
 * Normally this should be enabled.
 * @ingroup ic_defines */
#define IC_INIT_ENABLE_FILTER_INSTABILITY_RECOVERY  1
/** Ratio of the output to input at which the filter will reset.
 * Setting it to 2.0 or above is a good rule of thumb.
 * @ingroup ic_defines */
#define IC_INIT_INSTABILITY_RATIO_LIMIT             2.0
/** Alpha used for low pass filtering the voice chance estimate based on VAD input.
 * @ingroup ic_defines */
#define IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA         0.99
/** Slow alpha used filtering input and output energies of IC used in the adaption controller.
 * @ingroup ic_defines */
#define IC_INIT_ENERGY_ALPHA_SLOW                   0.999
/** Fast alpha used filtering input and output energies of IC used in the adaption controller.
 * @ingroup ic_defines */
#define IC_INIT_ENERGY_ALPHA_FAST                   0.98
/** Leakage alpha used in the case where instability is detected. This allows the filter to stabilise
 * without completely forgetting the adaption.
 * @ingroup ic_defines */
#define IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA  0.995
/** Initial smoothed voice chance at startup. This value is quickly replaced by the calculated
 * voice change value from the VAD signal.
 * @ingroup ic_defines */
#define IC_INIT_SMOOTHED_VOICE_CHANCE               0.999999999999999


///////////////////////////////////////////////////////////////////////////////////////////////
///////Parameters below are fixed and are note designed to be configurable - DO NOT EDIT///////
///////////////////////////////////////////////////////////////////////////////////////////////

/** Number of Y channels input. This is fixed at 1 for the IC. The Y channel is delayed and used to 
 * generate the estimated noise signal to subtract from X. In practical terms it does not matter
 * which microphone is X and which is Y. NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */
#define IC_Y_CHANNELS 1

/** Number of X channels input. This is fixed at 1 for the IC. The X channel is the microphone
 * from which the estimated noise signal is subtracted. In practical terms it does not matter
 * which microphone is X and which is Y. NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */
 #define IC_X_CHANNELS 1


/** Time domain samples block length used internally in the IC's block LMS algorithm. 
 * NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */
#define IC_FRAME_LENGTH 512 

/** @brief IC new samples frame size
 * This is the number of samples of new data that the IC works on every frame. 240 samples at 16kHz is 15msec. Every
 * frame, the IC takes in 15msec of mic data and generates 15msec of interference cancelled output.
 * NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */
#define IC_FRAME_ADVANCE 240

#define IC_FRAME_OVERLAP (IC_FRAME_LENGTH - (2 * IC_FRAME_ADVANCE))

/** Number of bins of spectrum data computed when doing a DFT of a IC_FRAME_LENGTH length time domain vector. The
 * IC_FD_FRAME_LENGTH spectrum values represent the bins from DC to Nyquist. NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */   
#define IC_FD_FRAME_LENGTH ((IC_FRAME_LENGTH / 2) + 1) //Frequency domain frame length

/** Extra 2 samples you need to allocate in time domain so that the full spectrum (DC to nyquist) can be stored
 * after the in-place FFT. NOT USER MODIFIABLE.
 *
 * @ingroup ic_defines
 */  
//
#define FFT_PADDING 2

//For unit tests
#ifdef __XC__ 
#undef DWORD_ALIGNED
#define DWORD_ALIGNED
#endif

//For the IC example
#if !PROFILE_PROCESSING
    #define prof(n, str)
    #define print_prof(start, end, framenum)
#endif

#endif
