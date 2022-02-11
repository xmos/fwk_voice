// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "ic_unit_tests.h"

 
void ic_adaption_controller_fp(
        uint8_t vad,
        double *smoothed_voice_chance,
        double voice_chance_alpha,
        double input_energy_slow,
        double output_energy_slow,
        double input_energy_fast,
        double output_energy_fast,
        double out_to_in_ratio_limit,
        int enable_filter_instability_recovery,
        int *reset,
        double instability_recovery_leakage_alpha,
        double *leakage_alpha,
        double *mu){

    double r = (double)vad/256;

    *smoothed_voice_chance = *smoothed_voice_chance * voice_chance_alpha;
    if(r > *smoothed_voice_chance){
        *smoothed_voice_chance = r;
    }
    // printf("fp r = %.22f\n", r);
    // printf("fp smoothed_voice_chance = %.22f\n", *smoothed_voice_chance);

    
    double mu_tmp = 1.0 - *smoothed_voice_chance;
    // printf("fp mu_tmp = %.22f\n", mu_tmp);

    double ratio = 1.0;
    if(input_energy_slow > 0.0){
        ratio = output_energy_slow / input_energy_slow;
    }
    // printf("fp ratio = %.22f\n", ratio);


    if (enable_filter_instability_recovery){
        if(ratio > out_to_in_ratio_limit){
            // printf("**fp ic_reset_filter** %f > %f\n", ratio, out_to_in_ratio_limit);
            *reset = 1;
        }
    }

    if(ratio >= 1.0){
        ratio = 1.0;
    } else {
        ratio = sqrt(ratio);
        ratio = sqrt(ratio);
    }
    // printf("fp ratio = %.22f\n", ratio);
    mu_tmp *= ratio;

    double fast_ratio = 1.0;
    if(input_energy_fast > 0.0){
        fast_ratio = output_energy_fast / input_energy_fast;
    }
    // printf("fp fast_ratio = %.22f\n", fast_ratio);
    // printf("fp mu = %.22f\n", mu_tmp);


    if(fast_ratio > 1.0){
        *leakage_alpha = instability_recovery_leakage_alpha;
        *mu = 0.0;
    }else{
        *leakage_alpha = 1.0;
        *mu = mu_tmp;
    }
}

int isclose(double ref_fp, double dut_fp, double rtol, double atol){
    double a_diff = (ref_fp - dut_fp) > 0 ? (ref_fp - dut_fp) : (dut_fp - ref_fp);

    double r_diff = 1.0;
    double abs_ref = fabs(ref_fp);
    double abs_dut = fabs(dut_fp);

    if(abs_dut > abs_ref){
        if(ref_fp > 0){
            r_diff = abs_dut/abs_ref - 1.0;
        }else{
            r_diff = INFINITY;
        }
    }else{
        if(abs_dut > 0){
            r_diff = abs_ref/abs_dut - 1.0;
        }else{
            r_diff = INFINITY;
        }
    }

    int isclose = 1;
    if(a_diff > atol && r_diff > rtol){
        printf("NOT CLOSE - ref: %lf, dut: %lf, rel: %lf(%lf) abs: %lf(%lf)\n", ref_fp, dut_fp, r_diff, rtol, a_diff, atol);
        isclose = 0;
    }else{
        // printf("CLOSE - ref: %lf, dut: %lf, rel: %lf(%lf) abs: %lf(%lf)\n", ref_fp, dut_fp, r_diff, rtol, a_diff, atol);

    }

    return isclose;
}


void check_parms_equal(char *str, int iter, double ref_fp, float_s32_t dut){
    double dut_fp = ldexp(dut.mant, dut.exp);

    double atol = 0.00001;
    double rtol = 0.00001;
   
    if(!isclose(ref_fp, dut_fp, rtol, atol)){
        printf("**%s** FAILED, iter: %d\n", str, iter);
        assert(0);
    }
}

int filter_is_reset(bfp_complex_s32_t H_hat_bfp[IC_Y_CHANNELS][IC_X_CHANNELS*IC_FILTER_PHASES]){
    int is_reset = 1;
    for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
        if(H_hat_bfp[0][ph].exp != -1024) is_reset = 0;
        if(H_hat_bfp[0][ph].hr != 31) is_reset = 0;
        for(int i=0; i<IC_FD_FRAME_LENGTH;i++){
            if(H_hat_bfp[0][ph].data[i].re != 0 || H_hat_bfp[0][ph].data[i].im != 0) is_reset = 0;
        }
    }
    return is_reset;
}

void test_adaption_controller() {
    ic_state_t state;
    ic_init(&state);
    ic_adaption_controller_state_t *st = &state.ic_adaption_controller_state;
    ic_adaption_controller_config_t *cf = &state.ic_adaption_controller_state.adaption_controller_config;

    unsigned seed = 55378008;

    double smoothed_voice_chance_fp = IC_INIT_SMOOTHED_VOICE_CHANCE;
    double voice_chance_alpha_fp = IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA;
    double out_to_in_ratio_limit_fp = IC_INIT_INSTABILITY_RATIO_LIMIT;
    int enable_filter_instability_recovery = 0;
    double instability_recovery_leakage_alpha_fp = IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA;
    double leakage_alpha_fp = IC_INIT_LEAKAGE_ALPHA;
    double mu_fp = IC_INIT_MU;

    st->smoothed_voice_chance = double_to_float_s32(smoothed_voice_chance_fp);
    cf->voice_chance_alpha = double_to_float_s32(voice_chance_alpha_fp);
    cf->out_to_in_ratio_limit = double_to_float_s32(out_to_in_ratio_limit_fp);
    cf->instability_recovery_leakage_alpha = double_to_float_s32(instability_recovery_leakage_alpha_fp);
    cf->leakage_alpha = double_to_float_s32(leakage_alpha_fp);
    state.mu[0][0] = double_to_float_s32(mu_fp);

    for(int iter=0; iter<(1<<11)/F; iter++) {

        const int exp_mod = 5;
        st->input_energy_slow.mant = pseudo_rand_int32(&seed);
        st->input_energy_slow.exp = pseudo_rand_int32(&seed) % exp_mod;
        st->output_energy_slow.mant = pseudo_rand_int32(&seed);
        st->output_energy_slow.exp = pseudo_rand_int32(&seed) % exp_mod;
        st->input_energy_fast.mant = pseudo_rand_int32(&seed);
        st->input_energy_fast.exp = pseudo_rand_int32(&seed) % exp_mod;
        st->output_energy_fast.mant = pseudo_rand_int32(&seed);
        st->output_energy_fast.exp = pseudo_rand_int32(&seed) % exp_mod;

        double input_energy_fp_slow = ldexp(st->input_energy_slow.mant, st->input_energy_slow.exp);
        double output_energy_fp_slow = ldexp(st->output_energy_slow.mant, st->output_energy_slow.exp);
        double input_energy_fp_fast = ldexp(st->input_energy_fast.mant, st->input_energy_fast.exp);
        double output_energy_fp_fast = ldexp(st->output_energy_fast.mant, st->output_energy_fast.exp);
        double instability_recovery_leakage_alpha_fp = ldexp(cf->instability_recovery_leakage_alpha.mant, cf->instability_recovery_leakage_alpha.exp);

        if(iter == 0 || iter == 1000){
            enable_filter_instability_recovery = 0;
            cf->enable_filter_instability_recovery = 0;
        }

        if(iter == 20){
            enable_filter_instability_recovery = 1;
            cf->enable_filter_instability_recovery = 1;
        }


        int has_reset = 0;

        uint8_t vad = pseudo_rand_int32(&seed) % 256;

        ic_adaption_controller(&state, vad);

        ic_adaption_controller_fp(
            vad,
            &smoothed_voice_chance_fp,
            voice_chance_alpha_fp,
            input_energy_fp_slow,
            output_energy_fp_slow,
            input_energy_fp_fast,
            output_energy_fp_fast,
            out_to_in_ratio_limit_fp,
            enable_filter_instability_recovery,
            &has_reset,
            instability_recovery_leakage_alpha_fp,
            &leakage_alpha_fp,
            &mu_fp);

        check_parms_equal("smoothed_voice_chance", iter, smoothed_voice_chance_fp, st->smoothed_voice_chance);
        check_parms_equal("mu", iter, mu_fp, state.mu[0][0]);
        check_parms_equal("leakage_alpha", iter, leakage_alpha_fp, cf->leakage_alpha);
      

    }
}