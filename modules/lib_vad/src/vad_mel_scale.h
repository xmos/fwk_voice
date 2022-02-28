// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef VAD_MEL_SCALE_H
#define VAD_MEL_SCALE_H

#include <stdint.h>
#include "vad_parameters.h"

#define VAD_N_MEL_SCALE 24
#define VAD_N_DCT 24

const extern uint32_t vad_mel_table24_512[257];

#endif
