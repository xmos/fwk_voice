// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

void update_td_ema_energy_fp(double *ema, double *input, double length, double alpha) {
    if(!length) return;
    double new_sample=0.0;
    for(int i=0; i<length; i++) {
        new_sample += input[i]*input[i];
    }
    *ema = (*ema * alpha) + ((1-alpha)*new_sample);
    //*output = new_sample;
}

#define TEST_LEN (AEC_PROC_FRAME_LENGTH + 1)
void test_update_td_ema_energy() {
    int32_t dut_mem[TEST_LEN];
    bfp_s32_t dut;
    bfp_s32_init(&dut, dut_mem, TEST_LEN, 0, 0);
    float_s32_t dut_ema;
    dut_ema.mant = 0;
    dut_ema.exp = -1024;
    double ref_ema = 0.0;

    double ref[TEST_LEN];
    
    unsigned seed = 5683;
    int max_diff = 0;
    for(int iter=0; iter<(1<<14)/F; iter++) {
        //input
        dut.exp = pseudo_rand_int(&seed, -31, 32);
        dut.hr = pseudo_rand_uint32(&seed) % 4;
        for(int i=0; i<TEST_LEN; i++) {
            dut.data[i] = pseudo_rand_int32(&seed) >> dut.hr;
            ref[i] = ldexp(dut.data[i], dut.exp);
        }

        //start offset
        unsigned start_offset = pseudo_rand_uint32(&seed) % TEST_LEN;
        unsigned leftover = TEST_LEN - start_offset;
        //length
        int length;
        if(leftover >= 1) {
            length = pseudo_rand_uint32(&seed) % leftover;
        }
        else if(leftover == 1) {
            length = 1;
        }
        else {
            continue;
        }

        //alpha
        uq2_30 alpha_q30;
        alpha_q30 = pseudo_rand_uint32(&seed) >> 1;
        //alpha_q30 = 1063004405;
        double alpha_fp = ldexp(alpha_q30, -30);

        //printf("iter %d. start_offset %d, leftover %d, length %d alpha %f\n",iter, start_offset, leftover, length, alpha_fp);


        update_td_ema_energy_fp(&ref_ema, &ref[start_offset], length, alpha_fp);
        
        //dut updates ema inplace
        aec_config_params_t cfg;
        cfg.aec_core_conf.ema_alpha_q30 = alpha_q30;
        aec_calc_time_domain_ema_energy(&dut_ema, &dut, start_offset, length, &cfg); 

        //printf("ref %f, dut %f\n",ref, ldexp(dut_ema.mant, dut_ema.exp));
        int dut = dut_ema.mant;
        int ref = double_to_int32(ref_ema, dut_ema.exp);
        //printf("ref 0x%x, dut 0x%x\n", ref, dut);
        int diff = ref - dut;
        if(diff < 0) diff = -diff;
        if(diff > max_diff) max_diff = diff;
        TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<12, ref, dut, "Output delta is too large");
    }
    printf("max_diff = %d\n",max_diff);
}

