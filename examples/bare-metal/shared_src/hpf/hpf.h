// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef HPF_API_H
#define HPF_API_H

#include "xmath/xmath.h"
#define AP_FRAME_ADVANCE (240)
#define BIQUAD_COUNT (2)
#define NUM_COEFF_PER_BIQUAD (5)
#define TOTAL_NUM_COEFF (BIQUAD_COUNT * NUM_COEFF_PER_BIQUAD)
#define FILT_EXP (-30)
#define NORM_EXP (-31)

/**
 * @brief Performs 100Hz High-Pass filtering
 * 
 * This function applies 2 biquad IIR filters on the 1.31 frame in-place.
 * 
 * @param[inout] data Array of the frame data on which to perform the hpf
 */
void pre_agc_hpf(int32_t data[AP_FRAME_ADVANCE]);

#endif
