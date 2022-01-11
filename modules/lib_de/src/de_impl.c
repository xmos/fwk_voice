// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "de_api.h"

#define DE_PROC_FRAME_LENGTH (512)
void de_calc_freq_domain_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input)
{
    int32_t DWORD_ALIGNED scratch_mem[DE_PROC_FRAME_LENGTH/2 + 1];
    assert(input->length <= DE_PROC_FRAME_LENGTH/2 + 1);
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, scratch_mem, 0, input->length, 0);
    bfp_complex_s32_squared_mag(&scratch, input);

    float_s64_t sum64 = bfp_s32_sum(&scratch);
    *fd_energy = float_s64_to_float_s32(sum64);
}

void estimate_delay (
        de_output_t *de_output,
        const bfp_complex_s32_t* H_hat, 
        unsigned num_phases)
{
    //Direct manipulation of mant/exp because double_to_float_s32(0.0) takes hundreds of cycles
    const float_s32_t zero = {0, 0};
    const float_s32_t one = {1, 0};

    float_s32_t peak_fd_power = zero;
    int32_t peak_power_phase_index = 0;
    de_output->sum_phase_powers = zero;
    
    for(int ph=0; ph<num_phases; ph++) { //compute delay over 1 x-y pair phases
        float_s32_t phase_power;
        de_calc_freq_domain_energy(&phase_power, &H_hat[ph]);
        de_output->phase_power[ph] = phase_power;
        de_output->sum_phase_powers = float_s32_add(de_output->sum_phase_powers, phase_power);
        if(float_s32_gt(phase_power, peak_fd_power)) {
            peak_fd_power = phase_power;
            peak_power_phase_index = ph;
        }
    }
    de_output->peak_phase_power = peak_fd_power;
    de_output->peak_power_phase_index = peak_power_phase_index;

    if(float_s32_gt(de_output->sum_phase_powers, zero)){
        float_s32_t num_phases_s32 = {num_phases, 0};
        de_output->peak_to_average_ratio = 
                float_s32_div(float_s32_mul(peak_fd_power, num_phases_s32), de_output->sum_phase_powers);
    }else{
        de_output->peak_to_average_ratio = one;
    }
    de_output->measured_delay = DE_FRAME_ADVANCE * peak_power_phase_index;
}
