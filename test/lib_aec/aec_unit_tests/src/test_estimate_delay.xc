// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
extern "C"{
    #include "aec_defines.h"
    #include "aec_api.h"
}

//Note this is larger than AEC_LIB_MAIN_FILTER_PHASES but AEC_LIB_MAX_Y_CHANNELS and AEC_LIB_MAX_X_CHANNELS are 2 so it works..
//i.e. 30 <= 10 * 2 * 2
#define NUM_PHASES_DELAY_EST    30
#define PHASE_CMPLX_AIR_LEN     257


//From test_cal_fd_frame_energy
extern void calc_fd_frame_energy_fp(double *output, dsp_complex_fp *input, int length);


int aec_estimate_delay_fp(  dsp_complex_fp H_hat[1][NUM_PHASES_DELAY_EST][PHASE_CMPLX_AIR_LEN], int32_t num_phases, int32_t len_phase, 
                            double *sum_phase_powers, double phase_powers[NUM_PHASES_DELAY_EST], double *peak_to_average_ratio,
                            double *peak_phase_power, int32_t *peak_power_phase_index){

    double peak_fd_power = 0.0;
    *peak_power_phase_index = 0;
    *sum_phase_powers = 0.0;

    for(int ch=0; ch<1; ch++) { //estimate delay for the first y-channel
        for(int ph=0; ph<num_phases; ph++) { //compute delay over 1 x-y pair phases
            double phase_power;
            calc_fd_frame_energy_fp(&phase_power, H_hat[ch][ph], len_phase);
            phase_powers[ph] = phase_power;
            // printf("ph %d power %lf\n",ph, phase_power);
            *sum_phase_powers += phase_power;
            if(phase_power > peak_fd_power){
                peak_fd_power = phase_power;
                *peak_power_phase_index = ph;
           }
       }
    }
    *peak_phase_power = peak_fd_power;
    if(*sum_phase_powers > 0){
        *peak_to_average_ratio = (*peak_phase_power * num_phases) / *sum_phase_powers;
    }else{
        *peak_to_average_ratio = 1.0;
    }
    // printf("peak_power_phase_index %d\n", *peak_power_phase_index);
    return AEC_FRAME_ADVANCE * *peak_power_phase_index;
}

#undef DWORD_ALIGNED
#define DWORD_ALIGNED [[aligned(8)]]

#define TEST_LEN (AEC_PROC_FRAME_LENGTH/2 + 1)
void test_delay_estimate() {
    unsafe {

    uint8_t DWORD_ALIGNED aec_memory_pool[sizeof(aec_memory_pool_t)];
    aec_state_t DWORD_ALIGNED state;
    aec_shared_state_t DWORD_ALIGNED shared_state;

    //FP version of phase coeffs
    dsp_complex_fp H_hat[1][NUM_PHASES_DELAY_EST][PHASE_CMPLX_AIR_LEN] = {{{{0.0}}}};

    const unsigned num_phases = 30;
    unsigned seed = 34575;
    unsigned ch = 0;

    //Populate selected phase with energy to see if we can read peak
    for(unsigned ph = 0; ph < num_phases; ph++){
        aec_init(&state, NULL, &shared_state, aec_memory_pool, NULL, 1, 1, num_phases, 0);
        memset(H_hat, 0, sizeof(H_hat));

        unsigned length = state.H_hat[ch][ph].length;
        TEST_ASSERT_EQUAL_INT32_MESSAGE(length, PHASE_CMPLX_AIR_LEN, "Phase length assumption wrong");


        state.H_hat[ch][ph].exp = att_random_int32(seed) % 40; //Between +39 -39
        for(unsigned i = 0; i < length; i++){
            state.H_hat[ch][ph].data[i].re = att_random_int32(seed);
            state.H_hat[ch][ph].data[i].im = att_random_int32(seed);

            H_hat[ch][ph][i].re = att_int32_to_double(state.H_hat[ch][ph].data[i].re, state.H_hat[ch][ph].exp);
            H_hat[ch][ph][i].im = att_int32_to_double(state.H_hat[ch][ph].data[i].im, state.H_hat[ch][ph].exp);

        }
        int measured_delay = aec_estimate_delay(&state.shared_state->delay_estimator_params, state.H_hat[0], state.num_phases);

        double sum_phase_powers;
        double phase_powers[NUM_PHASES_DELAY_EST];
        double peak_to_average_ratio;
        double peak_phase_power;
        int32_t peak_power_phase_index;
        int measured_delay_fp = aec_estimate_delay_fp(H_hat, NUM_PHASES_DELAY_EST, PHASE_CMPLX_AIR_LEN,
                                    &sum_phase_powers, phase_powers, &peak_to_average_ratio, &peak_phase_power, &peak_power_phase_index);

        int actual_delay = ph * AEC_FRAME_ADVANCE;
        // printf("test_delay_estimate: %d (%d), fin\n", measured_delay, actual_delay);

        //Now check some things. First actual delay estimate vs expected
        TEST_ASSERT_EQUAL_INT32_MESSAGE(measured_delay, actual_delay, "DUT Delay estimate incorrect");
        TEST_ASSERT_EQUAL_INT32_MESSAGE(measured_delay_fp, actual_delay, "REF Delay estimate incorrect");

        //Now check accuracy
        double dut_peak_phase_power_fp = att_int32_to_double(state.shared_state->delay_estimator_params.peak_phase_power.mant, state.shared_state->delay_estimator_params.peak_phase_power.exp);
        double dut_sum_phase_powers_fp = att_int32_to_double(state.shared_state->delay_estimator_params.sum_phase_powers.mant, state.shared_state->delay_estimator_params.sum_phase_powers.exp);
        double dut_peak_to_average_ratio_fp = att_int32_to_double(state.shared_state->delay_estimator_params.peak_to_average_ratio.mant, state.shared_state->delay_estimator_params.peak_to_average_ratio.exp);

        double sum_phase_powers_ratio = sum_phase_powers / dut_sum_phase_powers_fp;
        double peak_phase_power_ratio = peak_phase_power / dut_peak_phase_power_fp;
        double peak_to_average_ratio_ratio = peak_to_average_ratio / dut_peak_to_average_ratio_fp;

        // printf("exponent: %d\n", state.H_hat[ch][ph].exp);
        // printf("sum_phase_powers ref: %lf dut: %lf, ratio: %lf\n", sum_phase_powers, dut_sum_phase_powers_fp, sum_phase_powers_ratio);
        // printf("peak_phase_power ref: %lf dut: %lf, ratio: %lf\n", peak_phase_power, dut_peak_phase_power_fp, peak_phase_power_ratio);
        // printf("peak_to_average_ratio ref: %lf dut: %lf, ratio: %lf\n", peak_to_average_ratio, dut_peak_to_average_ratio_fp, peak_to_average_ratio_ratio);

        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, 1.0, (float)sum_phase_powers_ratio, "sum_phase_powers_ratio incorrect");
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, 1.0, (float)peak_phase_power_ratio, "peak_phase_power_ratio incorrect");
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, 1.0, (float)peak_to_average_ratio_ratio, "peak_to_average_ratio_ratio incorrect");
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, (float)NUM_PHASES_DELAY_EST, (float)dut_peak_to_average_ratio_fp, "peak_to_average_ratio_ratio incorrect");
    }

    //Now try a few corner cases
    
    aec_init(&state, NULL, &shared_state, aec_memory_pool, NULL, 1, 1, num_phases, 0);
    memset(H_hat, 0, sizeof(H_hat));

    double sum_phase_powers;
    double phase_powers[NUM_PHASES_DELAY_EST];
    double peak_to_average_ratio;
    double peak_phase_power;
    int32_t peak_power_phase_index;
    int measured_delay_fp = aec_estimate_delay_fp(H_hat, NUM_PHASES_DELAY_EST, PHASE_CMPLX_AIR_LEN,
                                &sum_phase_powers, phase_powers, &peak_to_average_ratio, &peak_phase_power, &peak_power_phase_index);
    int measured_delay = aec_estimate_delay(&state.shared_state->delay_estimator_params, state.H_hat[0], state.num_phases);
    double dut_peak_to_average_ratio_fp = att_int32_to_double(state.shared_state->delay_estimator_params.peak_to_average_ratio.mant, state.shared_state->delay_estimator_params.peak_to_average_ratio.exp);
    printf("peak_to_average_ratio ref: %lf dut: %lf\n", peak_to_average_ratio, dut_peak_to_average_ratio_fp);

    //Even though zero, should come out to 1 as no energy in H
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, 1.0, (float)dut_peak_to_average_ratio_fp, "dut_peak_to_average_ratio_fp incorrect");
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0, 1.0, (float)peak_to_average_ratio, "peak_to_average_ratio incorrect");


    }
}
