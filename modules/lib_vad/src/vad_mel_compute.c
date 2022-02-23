// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "vad_helpers.h"
#include "vad_mel.h"

#define LOOKUP_PRECISION 8
#define MEL_PRECISION 24

#if X86_BUILD
    //TODO Add x86 equivalents
void mul_mel(uint32_t * h, uint32_t * l, uint32_t scale) {}
void add_unsigned_hl(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l) {}
#else
static inline void mul_mel(uint32_t * h, uint32_t * l,
                 uint32_t scale) {
    uint32_t hi, li;
    uint32_t vl = *l;
    uint32_t vh = *h;
    asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (hi), "=r" (li) : "r" (scale), "r" (vl), "r" (0), "r" (0));
    asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (vh), "=r" (vl) : "r" (scale), "r" (vh), "r" (hi), "r" (0));
    *h = vh;
    *l = vl;
}

static inline void add_unsigned_hl(uint32_t * sumH, uint32_t * sumL,
                                uint32_t h, uint32_t l) {
    uint32_t cout, cou2;
    uint32_t vL = *sumL;
    uint32_t vH = *sumH;
    asm("ladd %0, %1, %2, %3, %4" : "=r" (cout), "=r" (vL) : "r" (vL), "r" (l), "r" (0));
    asm("ladd %0, %1, %2, %3, %4" : "=r" (cou2), "=r" (vH) : "r" (vH), "r" (h), "r" (cout));
    *sumL = vL;
    *sumH = vH;
}
#endif

static int lookup[33] = {
    0,    11,  22,  33,  43,  53,  63,  73,
    82,   91, 100, 109, 117, 125, 134, 141,
    149, 157, 164, 172, 179, 186, 193, 200,
    206, 213, 219, 225, 232, 238, 244, 250,
    256
};



int lookup_small_log2_linear_new(uint32_t x) {
    int mask_bits = 26;
    int mask = (1 << mask_bits) - 1;
    int y = (x >> mask_bits)-32;
    int y1 = y + 1;
    int v0 = lookup[y], v1 = lookup[y1];
    int f1 = x & mask;
    int f0 = mask + 1 - f1;
    return (v0 * (uint64_t) f0 + v1 * (uint64_t) f1) >> (mask_bits - (MEL_PRECISION - LOOKUP_PRECISION));
}

volatile uint32_t bits; // http://bugzilla.xmos.local/show_bug.cgi?id=18641
int log_exponent_new(uint32_t h, uint32_t l, uint32_t logN) {
    bits = clz(h);
    uint32_t x;
    uint32_t exponent;
    if (bits == 32) {
        bits = clz(l);
        if (bits == 32) {
            return -1000;
        }
        x = l << bits;
        exponent = 32 - bits;
    } else {
        x = h << bits | l >> (32 - bits);
        exponent = 64 - bits;
    }
    uint32_t log2 = lookup_small_log2_linear_new(x) + ((exponent-1 + logN) << MEL_PRECISION);
    uint32_t ln2 = 2977044472;
    return (log2 * (uint64_t) ln2) >> 32;
}

void vad_mel_compute_new(int32_t melValues[], uint32_t M,
                    complex_s32_t pts[], uint32_t N,
                    const uint32_t melTable[],
                    int32_t extraShift) {

    extraShift += (32 - VAD_MEL_BITS);
    
    uint32_t sumEvenH = 0, sumOddH = 0;
    uint32_t sumEvenL = 0, sumOddL = 0;
    int mels = 0;

    for(int i = 0; i < M; i++) {
        melValues[i] = 0;
    }

    for(int i = 0; i <= N; i++) {
        uint64_t s = pts[i].re * (uint64_t) pts[i].re + pts[i].im * (uint64_t) pts[i].im;
        uint32_t h = s >> 32;
        uint32_t l = s & 0xffffffff;
        uint32_t ho = h;
        uint32_t lo = l;

        uint32_t scale = melTable[i];
        if (scale == 0 && i != 0) {
            int log = log_exponent_new(sumEvenH, sumEvenL, extraShift);
            if(i<N)melValues[mels++] = log;
            sumEvenH = 0;
            sumEvenL = 0;
        } else {
            mul_mel(&h, &l, scale);
            add_unsigned_hl(&sumEvenH, &sumEvenL, h, l);
        }
        scale = VAD_MEL_MAX - scale;
        if (scale == 0) {
            int32_t log = log_exponent_new(sumOddH, sumOddL, extraShift);
            melValues[mels++] = log;
            sumOddH = 0;
            sumOddL = 0;
        } else {
            mul_mel(&ho, &lo, scale);
            add_unsigned_hl(&sumOddH, &sumOddL, ho, lo);
        }
    }
}


//To access the inlines from unit testsing
void mul_mel_unit_test(uint32_t * h, uint32_t * l, uint32_t scale) {
    mul_mel(h, l, scale);
}
void add_unsigned_hl_unit_test(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l) {
    add_unsigned_hl(sumH, sumL, h, l);
}

