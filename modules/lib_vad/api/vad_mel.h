// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_mel_h_
#define _vad_mel_h_

#include <stdint.h>
#include "dsp_complex.h"

#define VAD_MEL_BITS 24
#define VAD_MEL_MAX (1 << VAD_MEL_BITS)

extern void vad_mel_compute(int32_t melValues[], uint32_t M,
                           dsp_complex_t pts[], uint32_t N,
                           const uint32_t melTable[],
                           int32_t extra_shift) ;

extern int32_t vad_spectral_centroid_Hz(dsp_complex_t p[], uint32_t N);

extern int32_t vad_spectral_spread_Hz(dsp_complex_t p[], uint32_t N,
                                      int32_t spectral_centroid);

#endif
