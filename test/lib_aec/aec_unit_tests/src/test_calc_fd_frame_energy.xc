// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
extern "C"{
    #include "aec_defines.h"
    #include "aec_api.h"
}

void calc_fd_frame_energy_fp(double *output, dsp_complex_fp *input, int length) {
    *output = 0.0;
    for(int i=0; i<length; i++) {
        *output += ((input[i].re * input[i].re) + (input[i].im * input[i].im));
    }
}

#define TEST_LEN (AEC_PROC_FRAME_LENGTH/2 + 1)
void test_calc_fd_frame_energy() {
    unsafe {
    complex_s32_t mem[TEST_LEN];
    bfp_complex_s32_t dut_in;
    bfp_complex_s32_init(&dut_in, mem, 0, TEST_LEN, 0);
    dsp_complex_fp ref_in[TEST_LEN];
    
    unsigned seed = 34575;
    for(int iter = 0; iter<(1<<12)/F; iter++) {
        dut_in.exp = sext(att_random_int32(seed), 6);        
        dut_in.hr = att_random_uint32(seed) % 4;
        for(int i=0; i<TEST_LEN; i++) {
            dut_in.data[i].re = att_random_int32(seed) >> dut_in.hr;
            dut_in.data[i].im = att_random_int32(seed) >> dut_in.hr;

            ref_in[i].re = att_int32_to_double(dut_in.data[i].re, dut_in.exp);
            ref_in[i].im = att_int32_to_double(dut_in.data[i].im, dut_in.exp);
        }
        double ref_out;
        calc_fd_frame_energy_fp(&ref_out, ref_in, TEST_LEN);
        float_s32_t dut_out;
        aec_calc_freq_domain_energy(&dut_out, &dut_in); //this only works for input size AEC_PROC_FRAME_LENGTH/2 + 1 since there is a static allocation of scratch memory of this size within the function

        //printf("ref %f, dut %f\n",ref_out, att_int32_to_double(dut_out.mant, dut_out.exp));
        int32_t ref = att_double_to_int32(ref_out, dut_out.exp);
        int32_t dut = dut_out.mant;
        TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<2, ref, dut, "Output delta is too large");
    }
    }
}
