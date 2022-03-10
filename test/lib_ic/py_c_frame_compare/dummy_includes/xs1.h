// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _XS1_DUMMY_H_
#define _XS1_DUMMY_H_

#include <stdlib.h>
#include <stdint.h>

static inline int32_t get_random_number_dummy(unsigned *seed, unsigned poly)
{
    srand(*seed + poly);
    unsigned randn_bits = 0;
    unsigned rand_max = RAND_MAX;
    while(rand_max) {
        randn_bits ++;
        rand_max >>= 1;
    }
    unsigned x = 0;
    for (unsigned i=0; i<=32/randn_bits; i++) {
        x += rand() << (i*randn_bits);
    }
    *seed = (unsigned) x;
    return (int32_t) x;
}

#define crc32(x, y, z) get_random_number_dummy(&x, z)

#endif // _XS1_DUMMY_H_
