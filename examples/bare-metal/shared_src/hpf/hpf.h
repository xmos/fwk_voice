// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <bfp_math.h>
#define AP_FRAME_ADVANCE (240)
#define BIQUAD_COUNT (2)
#define NUM_COEFF_PER_BIQUAD (5)
#define TOTAL_NUM_COEFF (BIQUAD_COUNT * NUM_COEFF_PER_BIQUAD)

void pre_agc_hpf(int32_t data[AP_FRAME_ADVANCE]);
