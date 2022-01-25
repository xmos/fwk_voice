// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_DEFINES_H
#define IC_DEFINES_H

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


#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"

//User modifiable parameters
#define IC_INIT_SIGMA_XX_SHIFT                      11  //From XC and Python AEC
#define IC_INIT_GAMMA_LOG2                          1   //From Pyhton IC (2^1 = 2.0)
#define IC_INIT_DELTA                               0.0156249999963620211929 //From Python test_wav_ic
#define IC_INIT_MU                                  0.369566 //From Python test_wav_ic
#define IC_INIT_EMA_ALPHA                           0.9995117188 //From Pyhton model
#define IC_INIT_LEAKAGE_ALPHA                       1.0 //From Python model

#define IC_FILTER_PHASES                            10 //From Python model
#define IC_Y_CHANNEL_DELAY_SAMPS                    180 //From Python model. The group delay through the filter is 32 + this number


#define IC_INIT_ENABLE_FILTER_INSTABILITY_RECOVERY  1
#define IC_INIT_INSTABILITY_RATIO_LIMIT             2.0
#define IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA         0.99
#define IC_INIT_ENERGY_ALPHA_SLOW                   0.999
#define IC_INIT_ENERGY_ALPHA_FAST                   0.98
#define IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA  0.995

#define IC_INIT_SMOOTHED_VOICE_CHANCE               0.999999999999999


//Fixed paramaters - do not edit
#define IC_Y_CHANNELS 1
#define IC_X_CHANNELS 1

#define IC_FRAME_LENGTH 512 
#define IC_FRAME_ADVANCE 240
#define IC_FRAME_OVERLAP (IC_FRAME_LENGTH - (2 * IC_FRAME_ADVANCE))
#define IC_FD_FRAME_LENGTH ((IC_FRAME_LENGTH / 2) + 1) //Frequency domain frame length

//extra 2 samples you need to allocate in time domain so that the full spectrum (DC to nyquist) can be stored after the in-place DFT
#define FFT_PADDING 2

//For unit tests
#ifdef __XC__ 
#undef DWORD_ALIGNED
#define DWORD_ALIGNED
#endif


#if !PROFILE_PROCESSING
    #define prof(n, str)
    #define print_prof(start, end, framenum)
#endif

#endif