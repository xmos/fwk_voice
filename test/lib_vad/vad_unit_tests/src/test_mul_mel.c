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

    unsigned max = 0x1;
    for(int i=0; i<tests_per_hr * 31; i++){
        uint32_t scale = pseudo_rand_uint(&seed, 0, UINT_MAX);
        uint32_t h_ref = pseudo_rand_uint(&seed, 0, max);
        uint32_t l_ref = pseudo_rand_uint(&seed, 0, UINT_MAX);;
        uint32_t h_dut = h_ref;
        uint32_t l_dut = l_ref;

        mul_mel_unit_test(&h_ref, &l_ref, scale);
        mul_mel_sim(&h_dut, &l_dut, scale);
        
        // printf("%lu %lu %lu %lu\n", h_ref, l_ref, h_ref, h_dut);
        TEST_ASSERT_EQUAL_INT32(l_ref, l_dut);
        TEST_ASSERT_EQUAL_INT32(h_ref, h_dut);

        if(i != 0 && i % tests_per_hr == 0){
            max <<= 1;
        }
    }
    printf("MUL_MEL all good...\n");

}
