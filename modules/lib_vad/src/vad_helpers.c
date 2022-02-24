// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <xs3_math.h>

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

#define ONE_Q8_24    (1<<24)
#define ONE_OVER_LN2 24204406
#define EXP_C1  11632640

// Changing from Q8.24 format to Q2.30 format improves accuracy from 
// Q1.31 doesn't make a differency anymore
#define EXP_C2     -3560
#define P0_EXP   67108864
#define P1_EXP   1116769
#define Q0_EXP   (ONE_Q8_24 * 8)
#define Q1_EXP   13418331

static int32_t use_exp_float(float_s32_t fl, exponent_t exp)
{
    exponent_t exp_diff = fl.exp - exp;

    if (exp_diff > 0) {
        return fl.mant << exp_diff;
    } else if (exp_diff < 0) {
        return fl.mant >> -exp_diff;
    }

    return fl.mant;
}

int32_t vad_round(int32_t x, int q_format) {
    // x += 0.5, truncate franctional bits
    return (x + (1 << (q_format - 1))) & ~((1 << q_format) - 1);
}

int32_t vad_math_exp(int32_t x){

    float_s32_t temp, temp2, temp3;
    temp.exp = -24; temp2.exp = -24; temp3.exp = -24;

    if(x > 81403559) {
       return INT32_MAX;
    } else if (x < -279097919) {
       return 0;
    }

    temp.mant = x;
    temp2.mant = ONE_OVER_LN2;
    temp = float_s32_mul(temp, temp2);

    int32_t XN = use_exp_float(temp, -24);
    XN = vad_round(XN, -24);
    int32_t N = XN >> 24;

    temp.mant = N << 24; temp.exp = -24;
    temp2.mant = EXP_C1; temp2.exp = -24;
    temp2 = float_s32_mul(temp2, temp);

    temp3.mant = EXP_C2;
    temp3 = float_s32_mul(temp3, temp);

    int32_t g = x - use_exp_float(temp2, -24) - use_exp_float(temp3, -24);

    temp.mant = g; temp.exp = -24;
    temp2.mant = g << 2; temp2.exp = -24;

    temp = float_s32_mul(temp, temp2);

    int32_t z4 = use_exp_float(temp, -24);

    temp.mant = P1_EXP; temp.exp = -24;
    temp2.mant = z4; temp2.exp = -24;
    temp = float_s32_mul(temp, temp2);
    temp.mant = use_exp_float(temp, -24); temp.exp = -24;
    temp.mant += P0_EXP << 2;

    temp3.mant = g << 3; temp3.exp = -24;
    temp = float_s32_mul(temp, temp3);
    int32_t precise = use_exp_float(temp, -24);
    int32_t gP = (precise + 4) >> 3;

    temp.mant = Q1_EXP; temp.exp = -24;
    temp = float_s32_mul(temp, temp2);
    int32_t Q = use_exp_float(temp, -24) + (Q0_EXP << 2);

    temp.mant = precise; temp.exp = -24;
    temp2.mant = Q - gP; temp2.exp = -24;
    temp = float_s32_div(temp, temp2);
    int32_t r = use_exp_float(temp, -24) + (ONE_Q8_24 << 2);
    N -= 2;

    return N > 0 ? (r << N) + (1 << (N - 1)) : (r + (1 << (- N - 1))) >> - N;
}

int32_t vad_math_logistics(int32_t x) {
    float_s32_t temp, temp2;
    int negative = x < 0;
    if (!negative) {
        x = -x;
    }
    temp.mant = vad_math_exp(x) + ONE_Q8_24; temp.exp = -24;
    temp2.mant = ONE_Q8_24; temp2.exp = -24;
    temp = float_s32_div(temp2, temp);

    int32_t val = use_exp_float(temp, -24);
    //int32_t val = dsp_math_divide(ONE_Q8_24, dsp_math_exp(x) + ONE_Q8_24, 24);
    if (negative) {
        return ONE_Q8_24 - val;
    }
    return val;
}

int32_t log_slope[8] = {1015490930, 640498971, 297985800, 120120271, 46079377, 17219453, 6371555, 3717288};
int32_t log_offset[8] = {8388608, 9853420, 12529304, 14613666, 15770555, 16334225, 16588473, 16661050};

int32_t vad_math_logistics_fast(int32_t x){
    int32_t r11 = clz_sim(x);
    int32_t r1 = ~ x;
    int32_t r2 = r1 >> 24;
    int32_t r3;
    int32_t mask = 0x00ffffff;
    if(r11){
        r1 = x >> 24;
        r2 = x + 0;
        r11 = r1 >> 3;
        if(r11) return mask;
        r3 = log_slope[r1];
        x = log_offset[r1];
        int64_t acc = ((int64_t)x << 32) + (int64_t)r11;
        acc += (int64_t)r3 * (int64_t)r2;
        return acc >> 32;
    }
    r11 = r2 >>3;
    if(r11) return 0;
    r3 = log_slope[r2];
    r2 = log_offset[r2];
    int64_t acc = ((int64_t)r2 << 32) + (int64_t)r11;
    acc += (int64_t)r3 * (int64_t)r1;
    x = mask - (int32_t)(acc >> 32);
    return x;
}
