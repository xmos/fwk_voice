// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"

void aec_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        aec_shared_state_t *shared_state,
        uint8_t *main_mem_pool,
        uint8_t *shadow_mem_pool,
        unsigned num_y_channels,
        unsigned num_x_channels,
        unsigned num_main_filter_phases,
        unsigned num_shadow_filter_phases) {

    aec_priv_main_init(main_state, shared_state, main_mem_pool, num_y_channels, num_x_channels, num_main_filter_phases);
    aec_priv_shadow_init(shadow_state, shared_state, shadow_mem_pool, num_shadow_filter_phases);
}

void aec_frame_init(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2])
{
    unsigned num_y_channels = main_state->shared_state->num_y_channels;
    unsigned num_x_channels = main_state->shared_state->num_x_channels;
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        bfp_s32_init(&main_state->shared_state->y[ch], &y_data[ch][0], -31, AEC_PROC_FRAME_LENGTH, 1);
    }
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&main_state->shared_state->x[ch], &x_data[ch][0], -31, AEC_PROC_FRAME_LENGTH, 1);
    }
    //Keep a copy of y[240:480] in output which can later be used in calculate coherence, since original y will get overwritten by inplace error computation
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        memcpy(main_state->output[ch].data, &main_state->shared_state->y[ch].data[AEC_FRAME_ADVANCE], AEC_FRAME_ADVANCE*sizeof(int32_t));
        main_state->output[ch].exp = main_state->shared_state->y[ch].exp;
        main_state->output[ch].hr = main_state->shared_state->y[ch].hr;
    }
    
    //Initialise T
    //At the moment, there's only enough memory for storing num_x_channels and not num_y_channels*num_x_channels worth of T.
    //So T calculation cannot be parallelised across Y channels
    //Reuse X memory for calculating T
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&main_state->T[ch], (complex_s32_t*)&x_data[ch][0], 0, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
    }

    //set Y_hat memory to 0 since it will be used in bfp_complex_s32_macc operation in aec_l2_calc_Error_and_Y_hat()
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        main_state->Y_hat[ch].exp = -1024;
        main_state->Y_hat[ch].hr = 0;
        memset(&main_state->Y_hat[ch].data[0], 0, ((AEC_PROC_FRAME_LENGTH/2)+1)*sizeof(complex_s32_t));
    }
    if(shadow_state != NULL) {
        for(unsigned ch=0; ch<num_y_channels; ch++) {
            shadow_state->Y_hat[ch].exp = -1024;
            shadow_state->Y_hat[ch].hr = 0;
            memset(&shadow_state->Y_hat[ch].data[0], 0, ((AEC_PROC_FRAME_LENGTH/2)+1)*sizeof(complex_s32_t));
        }
    }
}

void aec_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const aec_config_params_t *conf)
{
    if(!length) {
        return;
    }
    bfp_s32_t input_chunk;
    bfp_s32_init(&input_chunk, &input->data[start_offset], input->exp, length, 0);
    input_chunk.hr = input->hr;
    float_s64_t dot64 = bfp_s32_dot(&input_chunk, &input_chunk);
    float_s32_t dot = float_s64_to_float_s32(dot64);
    *ema_energy = float_s32_ema(*ema_energy, dot, conf->aec_core_conf.ema_alpha_q30);
}

void aec_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input)
{
    //Input bfp_s32_t structure will get overwritten since FFT is computed in-place. Keep a copy of input->length and assign it back after fft call.
    //This is done to avoid having to call bfp_s32_init() on the input every frame
    int32_t len = input->length; 
    bfp_complex_s32_t *temp = bfp_fft_forward_mono(input);
    
    memcpy(output, temp, sizeof(bfp_complex_s32_t));
    bfp_fft_unpack_mono(output);
    input->length = len;
    return;
}

//per x-channel
//API: calculate X-energy (per x-channel)
void aec_update_total_X_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned recalc_bin) 
{
    if((state == NULL) || (!state->num_phases)) {
        return;
    }
 
    bfp_s32_t *X_energy_ptr = &state->X_energy[ch];
    bfp_complex_s32_t *X_ptr = &state->shared_state->X[ch];
    float_s32_t *max_X_energy_ptr = &state->max_X_energy[ch];
    aec_priv_update_total_X_energy(X_energy_ptr, max_X_energy_ptr, &state->shared_state->X_fifo[ch][0], X_ptr, state->num_phases, recalc_bin);
    return;
}
//per x-channel
void aec_update_X_fifo_and_calc_sigmaXX(
        aec_state_t *state,
        unsigned ch)
{
    bfp_s32_t *sigma_XX_ptr = &state->shared_state->sigma_XX[ch];
    bfp_complex_s32_t *X_ptr = &state->shared_state->X[ch];
    uint32_t sigma_xx_shift = state->shared_state->config_params.aec_core_conf.sigma_xx_shift;
    float_s32_t *sum_X_energy_ptr = &state->shared_state->sum_X_energy[ch]; //This needs to be done only for main filter, so doing it here instead of in aec_update_total_X_energy
    aec_priv_update_X_fifo_and_calc_sigmaXX(&state->shared_state->X_fifo[ch][0], sigma_XX_ptr, sum_X_energy_ptr, X_ptr, state->num_phases, sigma_xx_shift);
    return;
}

//per y-channel
void aec_calc_Error_and_Y_hat(
        aec_state_t *state,
        unsigned ch)
{
    if(state == NULL) {
        return;
    }
    bfp_complex_s32_t *Y_ptr = &state->shared_state->Y[ch];
    bfp_complex_s32_t *Y_hat_ptr = &state->Y_hat[ch];
    bfp_complex_s32_t *Error_ptr = &state->Error[ch];
    int32_t bypass_enabled = state->shared_state->config_params.aec_core_conf.bypass;
    aec_priv_calc_Error_and_Y_hat(Error_ptr, Y_hat_ptr, Y_ptr, state->X_fifo_1d, state->H_hat_1d[ch], state->shared_state->num_x_channels, state->num_phases, bypass_enabled);
}

void aec_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input)
{
    //Input bfp_complex_s32_t structure will get overwritten since IFFT is computed in-place. Keep a copy of input->length and assign it back after ifft call.
    //This is done to avoid having to call bfp_complex_s32_init() on the input every frame
    int32_t len = input->length;
    bfp_fft_pack_mono(input);
    bfp_s32_t *temp = bfp_fft_inverse_mono(input);
    memcpy(output, temp, sizeof(bfp_s32_t));

    input->length = len;
    return;
}

void aec_calc_coherence(
        aec_state_t *state,
        unsigned ch)
{
    if(state->shared_state->config_params.aec_core_conf.bypass) {
        return;
    }
    coherence_mu_params_t *coh_mu_state_ptr = &state->shared_state->coh_mu_state[ch];
    //We need y_hat[240:480] and y[240:480]
    bfp_s32_t y_hat_subset;
    bfp_s32_init(&y_hat_subset, &state->y_hat[ch].data[AEC_FRAME_ADVANCE], state->y_hat[ch].exp, AEC_FRAME_ADVANCE, 0);
    y_hat_subset.hr = state->y_hat[ch].hr;
    //y[240:480] has already been copied to state->output in aec_frame_init() since y memory stores Y now, so use state->output instead of y
    aec_priv_calc_coherence(coh_mu_state_ptr, &state->output[ch], &y_hat_subset, &state->shared_state->config_params);
    return;
}

void aec_create_output(
        aec_state_t *state,
        unsigned ch)
{
    if(state == NULL) {
        return;
    }
    bfp_s32_t *output_ptr = &state->output[ch];
    bfp_s32_t *overlap_ptr = &state->overlap[ch];
    bfp_s32_t *error_ptr = &state->error[ch];
    aec_priv_create_output(output_ptr, overlap_ptr, error_ptr, &state->shared_state->config_params);
    return;
}

void aec_calc_fd_frame_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input)
{
    int32_t DWORD_ALIGNED scratch_mem[AEC_PROC_FRAME_LENGTH/2 + 1];
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, scratch_mem, 0, AEC_PROC_FRAME_LENGTH/2 + 1, 0);
    bfp_complex_s32_squared_mag(&scratch, input);

    float_s64_t sum64 = bfp_s32_sum(&scratch);
    *fd_energy = float_s64_to_float_s32(sum64);
}

void aec_calc_inv_X_energy(
        aec_state_t *state,
        unsigned ch,
        unsigned is_shadow)
{
    if(state == NULL) {
        return;
    }
    //frequency smoothing
    //calc inverse energy
    bfp_s32_t *sigma_XX_ptr = &state->shared_state->sigma_XX[ch];
    bfp_s32_t *X_energy_ptr = &state->X_energy[ch];
    aec_priv_calc_inv_X_energy(&state->inv_X_energy[ch], X_energy_ptr, sigma_XX_ptr, &state->shared_state->config_params, state->delta, is_shadow);
    return;
}

void aec_filter_adapt(
        aec_state_t *state,
        unsigned y_ch)
{
    if(state == NULL) {
        return;
    }
    if(state->shared_state->config_params.aec_core_conf.bypass) {
        return;
    }
    bfp_complex_s32_t *T_ptr = &state->T[0];

    aec_priv_filter_adapt(state->H_hat_1d[y_ch], state->X_fifo_1d, T_ptr, state->shared_state->num_x_channels, state->num_phases);
}

void aec_compute_T(
        aec_state_t *state,
        unsigned y_ch,
        unsigned x_ch)
{
    if(state == NULL) {
        return;
    }
    bfp_complex_s32_t *T_ptr = &state->T[x_ch]; //Use the same memory as X to store T
    bfp_complex_s32_t *Error_ptr = &state->Error[y_ch];
    bfp_s32_t *inv_X_energy_ptr = &state->inv_X_energy[x_ch];
    float_s32_t mu = state->mu[y_ch][x_ch];
    aec_priv_compute_T(T_ptr, Error_ptr, inv_X_energy_ptr, mu);
}

void aec_compare_filters_and_calc_mu(
        aec_state_t *main_state,
        aec_state_t *shadow_state)
{
    if(main_state->shared_state->config_params.aec_core_conf.bypass) {
        return;
    }
    if(shadow_state != NULL) {
        aec_priv_compare_filters(main_state, shadow_state);
    }

    coherence_mu_params_t *coh_mu_state_ptr = main_state->shared_state->coh_mu_state;
    coherence_mu_config_params_t *coh_mu_conf_ptr = &main_state->shared_state->config_params.coh_mu_conf;
    aec_priv_calc_coherence_mu(coh_mu_state_ptr, coh_mu_conf_ptr, main_state->shared_state->sum_X_energy,
            main_state->shared_state->shadow_filter_params.shadow_flag, main_state->shared_state->num_y_channels, main_state->shared_state->num_x_channels);
    
    //calculate delta. Done here instead of aec_l2_calc_inv_X_energy_denom() since max_X_energy across all x-channels is needed in delta computation.
    //aec_l2_calc_inv_X_energy_denom() is called per x channel
    aec_priv_calc_delta(&main_state->delta, &main_state->max_X_energy[0], &main_state->shared_state->config_params, main_state->delta_scale, main_state->shared_state->num_x_channels);
    if(shadow_state != NULL) {
        aec_priv_calc_delta(&shadow_state->delta, &shadow_state->max_X_energy[0], &shadow_state->shared_state->config_params, shadow_state->delta_scale, shadow_state->shared_state->num_x_channels);
    }
    
    //Update main and shadow filter mu
    for(unsigned y_ch=0; y_ch<main_state->shared_state->num_y_channels; y_ch++) {
        for(unsigned x_ch=0; x_ch<main_state->shared_state->num_x_channels; x_ch++) {
            if(shadow_state != NULL) {
                shadow_state->mu[y_ch][x_ch] = shadow_state->shared_state->config_params.shadow_filt_conf.shadow_mu;
            }
            main_state->mu[y_ch][x_ch] = coh_mu_state_ptr[y_ch].coh_mu[x_ch];
        }
    }
}

void aec_update_X_fifo_1d(
        aec_state_t *state)
{
    if(state == NULL) {
        return;
    }
    unsigned count = 0;
    for(unsigned ch=0; ch<state->shared_state->num_x_channels; ch++) {
        for(unsigned ph=0; ph<state->num_phases; ph++) {
            state->X_fifo_1d[count] = state->shared_state->X_fifo[ch][ph];
            count += 1;
        }
    }
}

void aec_reset_filter(
        aec_state_t *state)
{
    for(int ch=0; ch<state->shared_state->num_y_channels; ch++) {
        aec_priv_reset_filter(state->H_hat_1d[ch], state->shared_state->num_x_channels, state->num_phases);
    }
}

#if 0
#include <xclib.h>
unsigned mk_mask(unsigned m){
    //(1<<m)-1
    asm volatile("mkmsk %0, %1":"=r"(m): "r"(m));
    return m;
}

void bfp_s32_calculate_min_mask(
        bfp_s32_t *input,
        unsigned *min_mask)
{
    *min_mask = 0;
    for(unsigned i=0; i<input->length; i++) {
        *min_mask |= mk_mask(clz(input->data[i]));
    }
}
#endif