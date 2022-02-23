// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>

// Naive implementation
int clz_sim(uint32_t x)
{
    int leading = 0;
    for(int i=31; i>=0; i--){
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
