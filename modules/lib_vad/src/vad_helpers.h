// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef VAD_HELPERS_H
#define VAD_HELPERS_H

int32_t vad_math_logistics_fast(int32_t x);

#if !X86_BUILD
#include <xclib.h>
#include "dsp.h"
//int32_t vad_math_logistics(int32_t x);
//int32_t vad_math_logistics_fast(int32_t x);
#else
int clz_sim(uint32_t x);
void mul_mel_sim(uint32_t * h, uint32_t * l, uint32_t scale);
void add_unsigned_hl_sim(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l);
//int32_t vad_math_logistics(int32_t x);
//TODO this is just to enable the x86 to build until we have this for x86
//#define dsp_math_logistics_fast(x) 0
//#define dsp_dct_forward24(dct_output, dct_input) 0
#endif

#endif