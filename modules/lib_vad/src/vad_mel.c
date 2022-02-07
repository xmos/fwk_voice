// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <vad_mel.h>
#include "vad_mel_scale.h"


#define LOOKUP_PRECISION 8
#define MEL_PRECISION 24

static int lookup[33] = {
    0,    11,  22,  33,  43,  53,  63,  73,
    82,   91, 100, 109, 117, 125, 134, 141,
    149, 157, 164, 172, 179, 186, 193, 200,
    206, 213, 219, 225, 232, 238, 244, 250,
    256
};

static inline int lookup_small_log2_linear(uint32_t x) {
    int mask_bits = 26;
    int mask = (1 << mask_bits) - 1;
    int y = (x >> mask_bits)-32;
    int y1 = y + 1;
    int v0 = lookup[y], v1 = lookup[y1];
    int f1 = x & mask;
    int f0 = mask + 1 - f1;
    return (v0 * (uint64_t) f0 + v1 * (uint64_t) f1) >> (mask_bits - (MEL_PRECISION - LOOKUP_PRECISION));
}

static inline int log_exponent(uint32_t val, uint32_t exp, uint32_t logN) {
    uint32_t bits = clz(val);
    uint32_t x = val<<bits;
    exp = exp - bits;
    /*uint32_t exponent;
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
    }*/

    uint32_t log2 = lookup_small_log2_linear(x) + ((exp-1 + logN) << MEL_PRECISION);
    uint32_t ln2 = 2977044472;
    return (log2 * (uint64_t) ln2) >> 32;
}

void vad_mel_compute(int32_t * melValues,
                    bfp_complex_s32_t * pts,
                    const int32_t melTb,
                    const int32_t melTb_rev, 
                    int32_t extra_shift){
    //mel shift

    bfp_s32_set(melValues, 0, VAD_EXP);

    bfp_s32_t power_spec, even, odd;
    int32_t power_data [VAD_PROC_FRAME_BINS];
    int32_t re_data [VAD_PROC_FRAME_BINS];
    float_s32_t sum_even, sum_odd;
    bfp_s32_init(&power_spec, power_data, VAD_EXP, VAD_PROC_FRAME_BINS, 0);
    bfp_s32_init(&even, melTb, VAD_EXP, VAD_PROC_FRAME_BINS, 1);
    bfp_s32_init(&odd, melTb_rev, VAD_EXP, VAD_PROC_FRAME_BINS, 1);

    bfp_complex_s32_macc(&power_spec, pts, pts);
    bfp_s32_mul(&even, &even, &power_spec);
    bfp_s32_mul(&odd, &odd, &power_spec);
    sum_even.exp = even.exp; sum_even.mant = 0;
    sum_odd.exp = odd.exp; sum_odd.mant = 0;
    int mels = 0;

    for(int v = 0; v < VAD_PROC_FRAME_BINS; v++){
        float_s32_t t;
        if(even.data[v] == 0 && v != 0){
            melValues[mels++] = log_exponent((uint32_t)sum_even.mant, sum_even.exp, extra_shift);
            sum_even.mant = 0;
            sum_even.exp = even.exp;
        } else {
            t.mant = even.data[v]; t.exp = even.exp;
            sum_even = float_s32_add(sum_even, t);
        }
        if(odd.data[v] == 0){
            melValues[mels++] = log_exponent((uint32_t)sum_odd.mant, sum_odd.exp, extra_shift);
            sum_odd.mant = 0;
            sum_odd.exp = odd.exp;
        } else {
            t.mant = odd.data[v]; t.exp = odd.exp;
            sum_odd = float_s32_add(sum_odd, t);
        }
    }
}