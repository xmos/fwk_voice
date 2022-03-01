// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_mel_h_
#define _vad_mel_h_

#include <stdint.h>
#include "bfp_math.h"

#define VAD_MEL_BITS 24
#define VAD_MEL_MAX (1 << VAD_MEL_BITS)

//For unit test
#ifdef __XC__
    #include "dsp.h"
#else
    void vad_mel_compute_new(int32_t melValues[], uint32_t M,
                               complex_s32_t pts[], uint32_t N,
                               const uint32_t melTable[],
                               int32_t extra_shift) ;
#endif
#endif
