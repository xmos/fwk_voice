// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AGC_DEFINES_H
#define AGC_DEFINES_H

#include "xmath/xmath.h"

// The input and output frame data format is Q1.31
#define FRAME_EXP -31

// Pre-calculated values to avoid the cycles of f32_to_float_s32()
#define FLOAT_S32_ZERO (float_s32_t){0, -31}
#define FLOAT_S32_ONE (float_s32_t){1073741824, -30}

// Alphas for EMA calculations are in Q30 format for float_s32_ema()
#define AGC_ALPHA_SLOW_RISE 952301632  // 0.8869
#define AGC_ALPHA_SLOW_FALL 1035731392  // 0.9646
#define AGC_ALPHA_FAST_RISE 409525120  // 0.3814
#define AGC_ALPHA_FAST_FALL 952301632  // 0.8869
#define AGC_ALPHA_PEAK_RISE 588410496  // 0.5480
#define AGC_ALPHA_PEAK_FALL 1035731392  // 0.9646
#define AGC_ALPHA_LC_EST_INC 588410496  // 0.5480
#define AGC_ALPHA_LC_EST_DEC 748720192  // 0.6973
#define AGC_ALPHA_LC_BG_POWER_EST_DEC 588410496  // 0.5480
#define AGC_ALPHA_LC_CORR 1052267008  // 0.9800

// Minimum value for the estimated far background power
#define AGC_LC_FAR_BG_POWER_EST_MIN (float_s32_t){1407374848, -47}  //0.00001

// Pre-calculated values for soft-clipping constants
#define AGC_SOFT_CLIPPING_THRESH (float_s32_t){1073741824, -31}  // 0.5
#define AGC_SOFT_CLIPPING_NUMERATOR (float_s32_t){1073741824, -32}  // 0.25; AGC_SOFT_CLIPPING_THRESH squared

#endif
