// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_mel_h_
#define _vad_mel_h_

#include <stdint.h>
#include "bfp_math.h"
#include "dsp.h"

#define VAD_MEL_BITS 24
#define VAD_MEL_MAX (1 << VAD_MEL_BITS)

extern void vad_mel_compute(int32_t * melValues,
                           bfp_complex_s32_t * pts,
                           const int32_t melTable,
                           const int32_t melTb_rev,
                           int32_t extra_shift) ;

extern int32_t vad_spectral_centroid_Hz(bfp_complex_s32_t * p, uint32_t N);

extern int32_t vad_spectral_spread_Hz(bfp_complex_s32_t * p, uint32_t N,
                                      int32_t spectral_centroid);

#endif

//vad_mel_compute(mel, VAD_N_MEL_SCALE+1, input, VAD_WINDOW_LENGTH/2, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
