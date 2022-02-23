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

    uint32_t sumH_ref = 0;
    uint32_t sumL_ref = 0;
    uint32_t sumH_dut = 0;
    uint32_t sumL_dut = 0;

    for(int i=0; i<tests_per_hr * 31; i++){
        uint32_t h = pseudo_rand_uint(&seed, 0, max);
        uint32_t l = pseudo_rand_uint(&seed, 0, UINT_MAX);;


        add_unsigned_hl_unit_test(&sumH_ref, &sumL_ref, h, l);
        add_unsigned_hl_sim(&sumH_dut, &sumL_dut, h, l);

        // printf("%lu %lu %lu %lu\n", sumH_ref, sumL_ref, sumH_dut, sumL_dut);
        TEST_ASSERT_EQUAL_INT32(sumL_ref, sumL_dut);
        TEST_ASSERT_EQUAL_INT32(sumH_ref, sumH_dut);

        if(i != 0 && i % tests_per_hr == 0){
            max <<= 1;
        }
    }
    printf("ADD_UNSIGNED all good...\n");


}
