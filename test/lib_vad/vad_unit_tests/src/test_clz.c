// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vad_unit_tests.h"
#include "vad_helpers.h"
#include <xclib.h>

extern int clz_sim(uint32_t x);
extern void mul_mel_unit_test(uint32_t * h, uint32_t * l, uint32_t scale);
extern void add_unsigned_hl_unit_test(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l);

extern void mul_mel_sim(uint32_t * h, uint32_t * l, uint32_t scale);
void add_unsigned_hl_sim(uint32_t * sumH, uint32_t * sumL, uint32_t h, uint32_t l);


void test(){
    unsigned tests_per_hr = 1000;
    unsigned seed = 6031759;

    unsigned max = UINT_MAX;
    for(int i=0; i<tests_per_hr * 31; i++){
        uint32_t tv = pseudo_rand_uint(&seed, 0, max);

        // http://bugzilla/show_bug.cgi?id=18641
        // ref = clz(tv);
        int ref = (tv == 0) ? 32 : clz(tv);
        int dut = clz_sim(tv);
        // printf("%d %d %d\n", tv, ref, dut);
        TEST_ASSERT_EQUAL_INT32(ref, dut);
        if(i != 0 && i % tests_per_hr == 0){
            max >>= 1;
        }
    }
    printf("CLZ all good...\n");
}
