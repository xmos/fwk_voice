// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef NN_COEFFICIENTS_H
#define NN_COEFFICIENTS_H

#include <stdint.h>
#include "vad_parameters.h"

#define N_VAD_INPUTS VAD_N_FEATURES
#define N_VAD_HIDDEN 25
#define N_VAD_OUTPUTS  1

const extern int32_t hidden_coeffs[N_VAD_HIDDEN * N_VAD_INPUTS + N_VAD_HIDDEN];
const extern int32_t outputs_coeffs[N_VAD_OUTPUTS * N_VAD_HIDDEN + N_VAD_OUTPUTS];

#define VAD_CLAMP(x) ((x) > 0x7fffffffULL \
                           ? 0x7fffffff \
                           : (x) < (-0x80000000LL) ? 0x80000000 : (int32_t)(x))

#define VAD_NORMALISE_COEFFICIENT(x) (VAD_CLAMP((x) * (1 << 24)))

#endif
