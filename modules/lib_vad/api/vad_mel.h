// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_mel_h_
#define _vad_mel_h_

#include <stdint.h>
#include "bfp_math.h"

#define VAD_MEL_BITS 24
#define VAD_MEL_MAX (1 << VAD_MEL_BITS)

extern void vad_mel_compute(int32_t melValues[], uint32_t M,
                           complex_s32_t pts[], uint32_t N,
                           const uint32_t melTable[],
                           int32_t extra_shift) ;

#endif

//vad_mel_compute(mel, VAD_N_MEL_SCALE+1, input, VAD_WINDOW_LENGTH/2, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
