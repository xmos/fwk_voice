// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "ic_unit_tests.h"
#include <stdio.h>
#include <xcore/assert.h>
#include "ic_api.h"

 
void ic_adaption_controller_fp(
        uint8_t vad,
        double *smoothed_voice_chance,
        double voice_chance_alpha,
        double input_energy,
        double output_energy,
        double input_energy0,
        double output_energy0,
        double out_to_in_ratio_limit,
        int enable_filter_instability_recovery,
        int *reset
        double instability_recovery_leakage_alpha,
        double *leakage_alpha,
        double *mu){

    double r = (double)vad/UCHAR_MAX;

    *smoothed_voice_chance = *smoothed_voice_chance * voice_chance_alpha;
    if(r > *smoothed_voice_chance){
        *smoothed_voice_chance = r;
    }

    printf("fp smoothed_voice_chance = %.22f\n", *smoothed_voice_chance);
    
    double mu = 1.0 - *smoothed_voice_chance;
    double ratio = 1.0;
    if(input_energy > 0.0){
        ratio = output_energy / input_energy;
    }

    if (enable_filter_instability_recovery){
        if(ratio  > out_to_in_ratio_limit){
            printf("**fp ic_reset_filter** %f > %f\n", ratio, out_to_in_ratio_limit);
            *reset = 1;
        }
    }

    if(ratio > 1.0){
        ratio = 1.0;
    } else {
        ratio = sqrt(ratio);
        ratio = sqrt(ratio);
    }
    *mu *= ratio;

    double fast_ratio = 1.0;
    if(input_energy0 > 0.0){
        fast_ratio = output_energy0 / input_energy;
    }

    if(fast_ratio > 1.0){
        *leakage_alpha = instability_recovery_leakage_alpha;
        *mu = 0.0;
    }else{
        *leakage_alpha = 1.0;
        *mu = *mu;
    }
}

int check_parms_equal(char *str, double ref_fp, float_s32_t dut){
    double dut_fp = att_int32_to_double(dut.mant, dut.exp);
    double diff = (ref_fp - dut_fp);
    double max_diff_percentage = 0.0;


    if(diff < 0.0) diff = -diff;
    double diff_percentage = (diff / (ref_fp < 0.0 ? -ref_fp : ref_fp)) * 100;
    if(diff_percentage > max_diff_percentage) max_diff_percentage = diff_percentage;
    if(diff > 0.0002*(ref_fp < 0.0 ? -ref_fp : ref_fp) + pow(10, -8))
    {
        printf("%s fail: iter %d, ych %d, ph %d, bin %d\n", str, iter, ych, ph, i);
        assert(0);
    }
}

int filter_is_reset(bfp_complex_s32_t H_hat_bfp[IC_Y_CHANNELS][IC_X_CHANNELS*IC_FILTER_PHASES]){
    int is_reset = 1;
    for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
        if(H_hat_bfp[0][ph].exp != -1024) is_reset = 0;
        if(H_hat_bfp[0][ph].hr != 31) is_reset = 0;
        for(int i=0; i<IC_FD_FRAME_LENGTH;i++){
            if(H_hat_bfp[0][ph].data[i] != 0) is_reset = 0;
        }
    }
    return is_reset;
}

void test_adaption_controller() {
    ic_state_t state;
    ic_init(&state);
    ic_adaption_controller_state_t *st = &state.ic_adaption_controller_state;

    unsigned seed = 45;

    double smoothed_voice_chance_fp = 0.0;
    double voice_chance_alpha_fp = 0.99;
    double out_to_in_ratio_limit_fp = 2.0;
    int enable_filter_instability_recovery = 0;
    int reset = 0;
    double instability_recovery_leakage_alpha_fp = 0.995;
    double leakage_alpha_fp = 1.0;
    double mu_fp = 0.34;

    st->smoothed_voice_chance = double_to_float_s32(smoothed_voice_chance_fp);
    st->voice_chance_alpha = double_to_float_s32(voice_chance_alpha_fp);
    st->out_to_in_ratio_limit = double_to_float_s32(out_to_in_ratio_limit_fp);
    st->instability_recovery_leakage_alpha = double_to_float_s32(instability_recovery_leakage_alpha_fp);
    st->leakage_alpha = double_to_float_s32(leakage_alpha_fp);
    st->mu = double_to_float_s32(mu_fp);

    for(int iter=0; iter<(1<<11)/F; iter++) {

        const int exp_mod = 5;
        st->input_energy.mant = att_random_uint32(seed);
        st->input_energy.exp = att_random_uint32(seed) % exp_mod;
        st->output_energy.mant = att_random_uint32(seed);
        st->output_energy.exp = att_random_uint32(seed) % exp_mod;
        st->input_energy0.mant = att_random_uint32(seed);
        st->input_energy0.exp = att_random_uint32(seed) % exp_mod;
        st->output_energy0.mant = att_random_uint32(seed);
        st->output_energy0.exp = att_random_uint32(seed) % exp_mod;

        double input_energy = att_int32_to_double(st->input_energy.mant, st->input_energy.exp);
        double output_energy = att_int32_to_double(st->output_energy.mant, st->output_energy.exp);
        double input_energy0 = att_int32_to_double(st->input_energy0.mant, st->input_energy0.exp);
        double output_energy0 = att_int32_to_double(st->output_energy0.mant, st->output_energy0.exp);

        uint8_t vad = att_random_uint32(seed) % 256;

        ic_adaption_controller(&state, vad);

        ic_adaption_controller_fp(
            vad,
            &smoothed_voice_chance_fp,
            voice_chance_alpha_fp,
            input_energy_fp,
            output_energy_fp,
            input_energy0_fp,
            output_energy0_fp,
            out_to_in_ratio_limit_fp,
            enable_filter_instability_recovery,
            &has_reset
            instability_recovery_leakage_alpha_fp,
            &leakage_alpha_fp,
            &mu_fp);

        check_parms_equal("smoothed_voice_chance", smoothed_voice_chance_fp, st->smoothed_voice_chance);
        check_parms_equal("mu", mu_fp, state.mu[0][0]);
        check_parms_equal("leakage_alpha", leakage_alpha_fp, st->leakage_alpha);
      

        if(iter == 20){
            enable_filter_instability_recovery = 1;
            st->enable_filter_instability_recovery = 1;
        }

    }
}