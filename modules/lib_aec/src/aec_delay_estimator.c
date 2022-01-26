// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "aec_defines.h"
#include "aec_api.h"

#if 0
int aec_estimate_delay (
        aec_state_t *state)
{
    //Direct manipulation of mant/exp because double_to_float_s32(0.0) takes hundreds of cycles
    const float_s32_t zero = {0, 0};
    const float_s32_t one = {1, 0};

    float_s32_t peak_fd_power = zero;
    int32_t peak_power_phase_index = 0;
    state->shared_state->delay_estimator_params.sum_phase_powers = zero;
    
    for(int ch=0; ch<1; ch++) { //estimate delay for the first y-channel
        for(int ph=0; ph<state->num_phases; ph++) { //compute delay over 1 x-y pair phases
            float_s32_t phase_power;
            aec_calc_freq_domain_energy(&phase_power, &state->H_hat[ch][ph]);
            state->shared_state->delay_estimator_params.phase_power[ph] = phase_power;
            state->shared_state->delay_estimator_params.sum_phase_powers = float_s32_add(state->shared_state->delay_estimator_params.sum_phase_powers, phase_power);
            if(float_s32_gt(phase_power, peak_fd_power)) {
                peak_fd_power = phase_power;
                peak_power_phase_index = ph;
            }
        }
    }
    state->shared_state->delay_estimator_params.peak_phase_power = peak_fd_power;
    state->shared_state->delay_estimator_params.peak_power_phase_index = peak_power_phase_index;

    if(float_s32_gt(state->shared_state->delay_estimator_params.sum_phase_powers, zero)){
        float_s32_t num_phases_s32 = {state->num_phases, 0};
        state->shared_state->delay_estimator_params.peak_to_average_ratio = 
                float_s32_div(float_s32_mul(peak_fd_power, num_phases_s32), state->shared_state->delay_estimator_params.sum_phase_powers);
    }else{
        state->shared_state->delay_estimator_params.peak_to_average_ratio = one;
    }

    return AEC_FRAME_ADVANCE * peak_power_phase_index;
}
#endif

int aec_estimate_delay (
        delay_estimator_params_t *de_state,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases)
{
    //Direct manipulation of mant/exp because double_to_float_s32(0.0) takes hundreds of cycles
    const float_s32_t zero = {0, 0};
    const float_s32_t one = {1, 0};

    float_s32_t peak_fd_power = zero;
    int32_t peak_power_phase_index = 0;
    de_state->sum_phase_powers = zero;
    
    for(int ph=0; ph<num_phases; ph++) { //compute delay over 1 x-y pair phases
        float_s32_t phase_power;
        aec_calc_freq_domain_energy(&phase_power, &H_hat[ph]);
        de_state->phase_power[ph] = phase_power;
        de_state->sum_phase_powers = float_s32_add(de_state->sum_phase_powers, phase_power);
        if(float_s32_gt(phase_power, peak_fd_power)) {
            peak_fd_power = phase_power;
            peak_power_phase_index = ph;
        }
    }
    de_state->peak_phase_power = peak_fd_power;
    de_state->peak_power_phase_index = peak_power_phase_index;

    if(float_s32_gt(de_state->sum_phase_powers, zero)){
        float_s32_t num_phases_s32 = {num_phases, 0};
        de_state->peak_to_average_ratio = 
                float_s32_div(float_s32_mul(peak_fd_power, num_phases_s32), de_state->sum_phase_powers);
    }else{
        de_state->peak_to_average_ratio = one;
    }

    return AEC_FRAME_ADVANCE * peak_power_phase_index;
}
