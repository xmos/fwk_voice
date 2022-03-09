// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <xs3_math.h>

// Naive implementation
int clz_sim(uint32_t x)
{
    int leading = 0;
    for(int i = 31; i >= 0; i--){
        if(x >> i){
            return leading;
        }
        leading++;
    }
}


/* LMUL NOTES
    op1, op4, op2, op3, op5, op6

    uint64_t result = (uint64_t)op2 * (uint64_t)op3;
    result = result + (uint64_t)op5 + (uint64_t)op6;
        
    op4 = (unsigned int)result;
    op1 = (unsigned int)(result >> 32);
*/

void mul_mel_sim(uint32_t * h, uint32_t * l, uint32_t scale) {
    
    // asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (hi), "=r" (li) : "r" (scale), "r" (l), "r" (0), "r" (0));
    uint64_t result = (uint64_t)(*l) * (uint64_t)scale;
    uint32_t hi = (uint32_t) (result >> 32);

    // asm("lmul %0, %1, %2, %3, %4, %5" : "=r" (h), "=r" (l) : "r" (scale), "r" (h), "r" (hi), "r" (0));
    result = (uint64_t)(*h) * (uint64_t)scale;
    result += hi;

    *h = (uint32_t)(result >> 32);
    *l = (uint32_t)result;
}


/* LADD NOTES
    op4, op1, op2, op3, op5

    uint64_t result = ((uint64_t)op2 + (uint64_t)op3) + ((uint64_t)op5 & 0x1);

    op1 = (unsigned int) result;
    op4 = (unsigned int) (result >> 32) & 0x1;      
*/

void add_unsigned_hl_sim(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l) {
    // uint32_t cout, cou2;
    // asm("ladd %0, %1, %2, %3, %4" : "=r" (cout), "=r" (sumL) : "r" (sumL), "r" (l), "r" (0));
    uint64_t result = (uint64_t)(*sumL) + (uint64_t)l;
    *sumL = (uint32_t)result;
    uint32_t cout = (uint32_t)(result >> 32);

    // asm("ladd %0, %1, %2, %3, %4" : "=r" (cou2), "=r" (sumH) : "r" (sumH), "r" (h), "r" (cout));
    result = (uint64_t)(*sumH) +  (uint64_t)h +  (uint64_t)(cout);

    *sumH = (uint32_t)result;     
}

const int32_t log_slope[8] = {1015490930, 640498971, 297985800, 120120271, 46079377, 17219453, 6371555, 3717288};
const int32_t log_offset[8] = {8388608, 9853420, 12529304, 14613666, 15770555, 16334225, 16588473, 16661050};

int32_t vad_math_logistics_fast(int32_t x){
    int32_t r11 = clz_sim(x);
    int32_t r1 = ~ x;
    int32_t r2 = r1 >> 24;
    // zero is default
    int32_t r3 = 0;
    int32_t mask = 0x00ffffff;
    if(r11){
        r1 = x >> 24;
        r2 = x + 0;
        r11 = r1 >> 3;
        if(r11){
            return mask;
        }
        r3 = log_slope[r1];
        x = log_offset[r1];
        int64_t acc = ((int64_t)x << 32) + (int64_t)r11;
        acc += (int64_t)r3 * (int64_t)r2;
        return (int32_t)(acc >> 32);
    }
    r11 = r2 >>3;
    if(r11){
        return 0;
    }
    r3 = log_slope[r2];
    r2 = log_offset[r2];
    int64_t acc = ((int64_t)r2 << 32) + (int64_t)r11;
    acc += (int64_t)r3 * (int64_t)r1;
    x = mask - (int32_t)(acc >> 32);
    return x;
}
