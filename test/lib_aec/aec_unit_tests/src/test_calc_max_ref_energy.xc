// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
extern "C"{
    #include "aec_api.h"
    float_s32_t aec_calc_max_ref_energy_c_wrapper(int32_t (*input)[AEC_FRAME_ADVANCE], int channels);
}

double calc_max_ref_energy_fp(double (*input)[AEC_FRAME_ADVANCE], int channels) {
    double max=0.0;
    for(int ch=0; ch<channels; ch++) {
        double current = 0.0;
        for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
            current += input[ch][i]*input[ch][i];
        }
        if(current > max) {max = current;}
    }
    return max;
}
#define CHANNELS (4)
void test_calc_max_ref_energy() {
    unsafe {
    int32_t [[aligned(8)]] dut[CHANNELS][AEC_FRAME_ADVANCE];
    float_s32_t dut_max;
    double ref[CHANNELS][AEC_FRAME_ADVANCE];
    double ref_max;
    
    unsigned seed = 568762;
    int max_diff = 0;
    for(int iter=0; iter<(1<<12)/F; iter++) {
        //input
        for(int ch=0; ch<CHANNELS; ch++) {
            int hr = att_random_uint32(seed) % 12;
            for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
                dut[ch][i] = att_random_int32(seed) >> hr;
                ref[ch][i] = att_int32_to_double(dut[ch][i], -31);
            }
        }
        ref_max = calc_max_ref_energy_fp(ref, CHANNELS);
        // xc wouldn't allow passing as pointer to const data so added wrapper c file as quick workaround. This will all be
        // cleaned up once unit tests are ported to c. 
        dut_max = aec_calc_max_ref_energy_c_wrapper(dut, CHANNELS); 

        int dut = dut_max.mant;
        int ref = att_double_to_int32(ref_max, dut_max.exp);
        //printf("ref 0x%x, dut 0x%x\n", ref, dut);
        int32_t diff = ref - dut;
        if(diff < 0) diff = -diff;
        if(diff > max_diff) max_diff = diff;
        TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<5, ref, dut, "Output delta is too large");
    }
    //printf("max_diff = %d\n",max_diff);
    }
}
