// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "vad_dct.h"

static const int32_t costable6[3] = {
    2074309917,
    1518500250,
    555809667,
};

static const int32_t costable12[6] = {
    2129111628,
    1984016189,
    1703713325,
    1307305214,
    821806413,
    280302863,
};

static const int32_t costable24[12] = {
    2142885721,
    2106220352,
    2033516969,
    1926019547,
    1785567396,
    1614563692,
    1415934356,
    1193077991,
    949807730,
    690285996,
    418953276,
    140452151,
};

static inline int32_t mulcos(int32_t x, int32_t cos) {
    long long r = cos * (long long) x;
    return r >> 31;
}

#define DCT(N,M)                                \
void vad_dct_forward##N(int32_t output[N], int32_t input[N]) { \
    int32_t temp[N/2], temp2[N/2]; \
    for(int32_t i = 0; i < N/2; i++) { \
        temp[i] = input[i] + input[N-1-i]; \
    } \
    vad_dct_forward##M(temp2, temp); \
    for(int32_t i = 0; i < N/2; i++) { \
        output[2*i] = temp2[i]; \
    } \
    for(int32_t i = 0; i < N/2; i++) { \
        int32_t z = input[i] - input[N-1-i]; \
        temp[i] = mulcos(z, costable##N[i]); \
    } \
    vad_dct_forward##M(temp2, temp); \
    int32_t last = temp2[0]; \
    output[1] = last; \
    for(int32_t i = 1; i < N/2; i++) { \
        last = temp2[i]*2 - last; \
        output[2*i+1] = last; \
    } \
}

void vad_dct_forward3(int32_t output[3], int32_t input[3]) {
    output[0] = input[0] + input[1] + input[2];
    output[1] = mulcos(input[0] - input[2], 1859775393);
    output[2] = ((input[0]+input[2])>>1) - input[1];
}

DCT(6,3)
DCT(12,6)
DCT(24,12)
