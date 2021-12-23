// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_DEFINES_H
#define IC_DEFINES_H

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "bfp_math.h"
#include "xs3_math.h"
#define Q1_30(f) ((int32_t)((double)(INT_MAX>>1) * f)) //TODO use lib_xs3_math use_exponent instead


//User modifiable parameters
#define IC_INIT_SIGMA_XX_SHIFT                      11 //From XC (no concept of this in Py)
// #define IC_INIT_SIGMA_XX_SHIFT                      0
// #define IC_INIT_GAMMA_LOG2                          2 //From XC IC
// #define IC_INIT_GAMMA_LOG2                          6 //From AEC
#define IC_INIT_GAMMA_LOG2                          1 //From Pyhton IC (2^1 = 2.0)

// #define IC_INIT_DELTA                               double_to_float_s32(0.00007) //XC
// #define IC_INIT_DELTA                               double_to_float_s32(0.000000007450581) //Python IC.py
#define IC_INIT_DELTA                               double_to_float_s32(0.0156249999963620211929) //Python test_wav_ic
#define IC_INIT_LEAKAGE_ALPHA                       double_to_float_s32(1.0)
#define IC_INIT_MU                                  double_to_float_s32(0.369566)
#define IC_INIT_EMA_ALPHA_Q30                       Q1_30(0.98) //XC
// #define IC_INIT_EMA_ALPHA_Q30                       Q1_30(0.9995117188) //Pyhton
#define IC_FILTER_PHASES                            (10)


#define IC_INIT_ENABLE_FILTER_INSTABILITY_RECOVERY  0
#define IC_INIT_SMOOTHED_VOICE_CHANCE               double_to_float_s32(0.999999999999999)
#define IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA         double_to_float_s32(0.99)
#define IC_INIT_ENERGY_ALPHA                        double_to_float_s32(0.999)
#define IC_INIT_ENERGY_ALPHA0                       double_to_float_s32(0.98)
#define IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA  double_to_float_s32(0.995)

//Fixed paramaters - do not edit
#define IC_Y_CHANNELS (1)
#define IC_X_CHANNELS (1)

#define IC_FRAME_LENGTH (512)   //Time domain frame length
#define IC_FRAME_ADVANCE (240)
#define IC_FRAME_OVERLAP (IC_FRAME_LENGTH - (2 * IC_FRAME_ADVANCE))
#define IC_FD_FRAME_LENGTH ((IC_FRAME_LENGTH / 2) + 1) //Frequency domain frame length

#define IC_TOTAL_INPUT_CHANNEL_PAIRS ((IC_Y_CHANNELS + IC_X_CHANNELS + 1) / 2)
#define IC_INPUT_CHANNEL_PAIRS IC_TOTAL_INPUT_CHANNEL_PAIRS
#define IC_TOTAL_OUTPUT_CHANNEL_PAIRS 1 //FIXME
#define IC_TOTAL_OUTPUT_CHANNELS 2

//extra 2 samples you need to allocate in time domain so that the full spectrum (DC to nyquist) can be stored after the in-place DFT
#define FFT_PADDING 2


#if !PROFILE_PROCESSING
    #define prof(n, str)
    #define print_prof(start, end, framenum)
#endif

#endif