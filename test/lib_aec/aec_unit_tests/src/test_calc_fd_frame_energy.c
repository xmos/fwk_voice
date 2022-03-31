// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

static void calc_fd_frame_energy_fp(double *output, complex_double_t *input, int length) {
    *output = 0.0;
    for(int i=0; i<length; i++) {
        *output += ((input[i].re * input[i].re) + (input[i].im * input[i].im));
    }
}

#define TEST_LEN (AEC_PROC_FRAME_LENGTH/2 + 1)
void test_calc_fd_frame_energy() {
    complex_s32_t mem[TEST_LEN];
    bfp_complex_s32_t dut_in;
    bfp_complex_s32_init(&dut_in, mem, 0, TEST_LEN, 0);
    complex_double_t ref_in[TEST_LEN];
    
    unsigned seed = 34575;
    for(int iter = 0; iter<(1<<12)/F; iter++) {
        dut_in.exp = pseudo_rand_int(&seed, -31, 32);        
        dut_in.hr = pseudo_rand_uint32(&seed) % 4;
        for(int i=0; i<TEST_LEN; i++) {
            dut_in.data[i].re = pseudo_rand_int32(&seed) >> dut_in.hr;
            dut_in.data[i].im = pseudo_rand_int32(&seed) >> dut_in.hr;

            ref_in[i].re = ldexp(dut_in.data[i].re, dut_in.exp);
            ref_in[i].im = ldexp(dut_in.data[i].im, dut_in.exp);
        }
        double ref_out;
        calc_fd_frame_energy_fp(&ref_out, ref_in, TEST_LEN);
        float_s32_t dut_out;
        aec_calc_freq_domain_energy(&dut_out, &dut_in); //this only works for input size AEC_PROC_FRAME_LENGTH/2 + 1 since there is a static allocation of scratch memory of this size within the function

        int32_t ref = double_to_int32(ref_out, dut_out.exp);
        int32_t dut = dut_out.mant;
        TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<2, ref, dut, "Output delta is too large");
    }
}
