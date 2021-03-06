// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef VAD_HELPERS_H
#define VAD_HELPERS_H

int32_t vad_math_logistics_fast(int32_t x);

#if !VAD_MODULE_X86_BUILD
    #include <xclib.h>
#else
    int clz_sim(uint32_t x);
    void mul_mel_sim(uint32_t * h, uint32_t * l, uint32_t scale);
    void add_unsigned_hl_sim(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l);
#endif

#endif
