// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef VAD_NORMALISATION_H
#define VAD_NORMALISATION_H

#include <stdint.h>
#include "vad_parameters.h"

#define FEATURE_MU_Q 16
#define FEATURE_SIGMA_Q 16

#define FEATURE_MU_SCALE(x)  ((int32_t)(x*(1<<FEATURE_MU_Q)))
#define FEATURE_SIGMA_SCALE(x)  ((int32_t)(x*(1<<FEATURE_SIGMA_Q)))
#define FEATURE_MU_SCALE_0(x)  ((int32_t)(x))
#define FEATURE_SIGMA_SCALE_0(x)  ((int32_t)(x))

extern const int32_t vad_mus[VAD_N_FEATURES_PER_FRAME];
extern const int32_t vad_sigmas[VAD_N_FEATURES_PER_FRAME];

#endif
