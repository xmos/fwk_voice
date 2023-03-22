// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "hpf_test.h"

#define len 240

extern const q2_30 hpf_coef_q30[TOTAL_NUM_COEFF];

void biquad(double out[len + 2], const double in[len + 2], const double coef[NUM_COEFF_PER_BIQUAD]){
    for(int v = 2; v < len + 2; v++){
        out[v] = in[v] * coef[0] + in[v - 1] * coef[1] + in[v - 2] * coef[2] + out[v - 1] * coef[3] + out[v - 2] * coef[4];
    }
}

void test_hpf_comp_double(){
    unsigned seed = 3542675;
    int32_t DWORD_ALIGNED samples_act[len] = {0};
    double coef_db[TOTAL_NUM_COEFF];
    double th = ldexp(1, -22);

    for(int v = 0; v < TOTAL_NUM_COEFF; v++){
        float_s32_t t;
        t.mant = hpf_coef_q30[v];
        t.exp = FILT_EXP;
        coef_db[v] = float_s32_to_double(t);
    }

    for(int i = 0; i < 100; i++){

        double orig_db[len + 2] = {0.0};
        double stage_1_out_db[len + 2] = {0.0};
        double expected_db[len + 2] = {0.0};

        for(int v = 0; v < len; v++){
            float_s32_t t;
            samples_act[v] = pseudo_rand_int(&seed, INT_MIN/2, INT_MAX/2);
            t.mant = samples_act[v];
            t.exp = NORM_EXP;
            orig_db[v + 2] = float_s32_to_double(t);
        }
    
        biquad(stage_1_out_db, orig_db, coef_db);
        biquad(expected_db, stage_1_out_db, &coef_db[NUM_COEFF_PER_BIQUAD]);

        pre_agc_hpf(samples_act);

        for(int v = 0; v < len; v++){
            float_s32_t t = {samples_act[v], NORM_EXP};
            double actual = float_s32_to_double(t);
            double error = fabs(expected_db[v + 2] - actual);
            TEST_ASSERT(error < th);
        }
    }
}
