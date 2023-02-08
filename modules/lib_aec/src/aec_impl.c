// Copyright 2022 XMOS LIMITED.
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
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE])
{
    unsigned num_y_channels = main_state->shared_state->num_y_channels;
    unsigned num_x_channels = main_state->shared_state->num_x_channels;

    // y frame 
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        /* Create 512 samples frame */
        // Copy previous y samples
        memcpy(main_state->shared_state->y[ch].data, main_state->shared_state->prev_y[ch].data, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy current y samples
        memcpy(&main_state->shared_state->y[ch].data[AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE], &y_data[ch][0], (AEC_FRAME_ADVANCE)*sizeof(int32_t));
        // Update exp just in case
        main_state->shared_state->y[ch].exp = AEC_INPUT_EXP;
        // Update headroom
        bfp_s32_headroom(&main_state->shared_state->y[ch]);

        /* Update previous samples */
        // Copy the last 32 samples to the beginning
        memcpy(main_state->shared_state->prev_y[ch].data, &main_state->shared_state->prev_y[ch].data[AEC_FRAME_ADVANCE], (AEC_PROC_FRAME_LENGTH - (2*AEC_FRAME_ADVANCE))*sizeof(int32_t));
        // Copy current frame to previous
        memcpy(&main_state->shared_state->prev_y[ch].data[(AEC_PROC_FRAME_LENGTH - (2*AEC_FRAME_ADVANCE))], &y_data[ch][0], AEC_FRAME_ADVANCE*sizeof(int32_t));
        // Update headroom
        bfp_s32_headroom(&main_state->shared_state->prev_y[ch]);
        // Update exp just in case
        main_state->shared_state->prev_y[ch].exp = AEC_INPUT_EXP;
    }
    // x frame 
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        /* Create 512 samples frame */
        // Copy previous x samples
        memcpy(main_state->shared_state->x[ch].data, main_state->shared_state->prev_x[ch].data, (AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy current x samples
        memcpy(&main_state->shared_state->x[ch].data[AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE], &x_data[ch][0], (AEC_FRAME_ADVANCE)*sizeof(int32_t));
        // Update exp just in case
        main_state->shared_state->x[ch].exp = AEC_INPUT_EXP;
        // Update headroom
        bfp_s32_headroom(&main_state->shared_state->x[ch]);

        /* Update previous samples */
        // Copy the last 32 samples to the beginning
        memcpy(main_state->shared_state->prev_x[ch].data, &main_state->shared_state->prev_x[ch].data[AEC_FRAME_ADVANCE], (AEC_PROC_FRAME_LENGTH - (2*AEC_FRAME_ADVANCE))*sizeof(int32_t));
        // Copy current frame to previous
        memcpy(&main_state->shared_state->prev_x[ch].data[(AEC_PROC_FRAME_LENGTH - (2*AEC_FRAME_ADVANCE))], &x_data[ch][0], AEC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        main_state->shared_state->prev_x[ch].exp = AEC_INPUT_EXP;
        // Update headroom
        bfp_s32_headroom(&main_state->shared_state->prev_x[ch]);
    }

    //Initialise T
    //At the moment, there's only enough memory for storing num_x_channels and not num_y_channels*num_x_channels worth of T.
    //So T calculation cannot be parallelised across Y channels
    //Reuse X memory for calculating T
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&main_state->T[ch], (complex_s32_t*)&main_state->shared_state->x[ch].data[0], 0, (AEC_PROC_FRAME_LENGTH/2)+1, 0);
    }

    //set Y_hat memory to 0 since it will be used in bfp_complex_s32_macc operation in aec_l2_calc_Error_and_Y_hat()
    for(unsigned ch=0; ch<num_y_channels; ch++) {
        main_state->Y_hat[ch].exp = AEC_ZEROVAL_EXP;
        main_state->Y_hat[ch].hr = AEC_ZEROVAL_HR;
        memset(&main_state->Y_hat[ch].data[0], 0, ((AEC_PROC_FRAME_LENGTH/2)+1)*sizeof(complex_s32_t));
    }
    if(shadow_state != NULL) {
        for(unsigned ch=0; ch<num_y_channels; ch++) {
            shadow_state->Y_hat[ch].exp = AEC_ZEROVAL_EXP;
            shadow_state->Y_hat[ch].hr = AEC_ZEROVAL_HR;
            memset(&shadow_state->Y_hat[ch].data[0], 0, ((AEC_PROC_FRAME_LENGTH/2)+1)*sizeof(complex_s32_t));
        }
    }
}

void aec_calc_time_domain_ema_energy(
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
    bfp_s32_init(&input_chunk, &input->data[start_offset], input->exp, length, 1);
    float_s64_t dot64 = bfp_s32_dot(&input_chunk, &input_chunk);
    float_s32_t dot = float_s64_to_float_s32(dot64);
    *ema_energy = float_s32_ema(*ema_energy, dot, conf->aec_core_conf.ema_alpha_q30);
}

float_s32_t aec_calc_max_input_energy(const int32_t (*input_data)[AEC_FRAME_ADVANCE], int num_channels) {
    bfp_s32_t ref;

    bfp_s32_init(&ref, (int32_t*)&input_data[0][0], AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 1);
    float_s32_t max = float_s64_to_float_s32(bfp_s32_energy(&ref));
    for(int ch=1; ch<num_channels; ch++) {
        bfp_s32_init(&ref, (int32_t*)&input_data[ch][0], AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 1);
        float_s32_t current = float_s64_to_float_s32(bfp_s32_energy(&ref));
        if(float_s32_gt(current, max)){max = current;}
    }
    return max;
}

void aec_forward_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input)
{
    //Input bfp_s32_t structure will get overwritten since FFT is computed in-place. Keep a copy of input->length and assign it back after fft call.
    //This is done to avoid having to call bfp_s32_init() on the input every frame
    uint32_t len = input->length; 
    bfp_complex_s32_t *temp = bfp_fft_forward_mono(input);
    temp->hr = bfp_complex_s32_headroom(temp); // TODO Workaround till https://github.com/xmos/lib_xcore_math/issues/96 is fixed
    
    memcpy(output, temp, sizeof(bfp_complex_s32_t));
    bfp_fft_unpack_mono(output);
    input->length = len;
}

//per x-channel
//API: calculate X-energy (per x-channel)
void aec_calc_X_fifo_energy(
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
}
//per x-channel
void aec_update_X_fifo_and_calc_sigmaXX(
        aec_state_t *state,
        unsigned ch)
{
    bfp_s32_t *sigma_XX_ptr = &state->shared_state->sigma_XX[ch];
    bfp_complex_s32_t *X_ptr = &state->shared_state->X[ch];
    uint32_t sigma_xx_shift = state->shared_state->config_params.aec_core_conf.sigma_xx_shift;
    float_s32_t *sum_X_energy_ptr = &state->shared_state->sum_X_energy[ch]; //This needs to be done only for main filter, so doing it here instead of in aec_calc_X_fifo_energy
    aec_priv_update_X_fifo_and_calc_sigmaXX(&state->shared_state->X_fifo[ch][0], sigma_XX_ptr, sum_X_energy_ptr, X_ptr, state->num_phases, sigma_xx_shift);
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
    aec_priv_calc_Error_and_Y_hat(Error_ptr, Y_hat_ptr, Y_ptr, state->X_fifo_1d, state->H_hat[ch], state->shared_state->num_x_channels, state->num_phases, bypass_enabled);
}

void aec_inverse_fft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input)
{
    //Input bfp_complex_s32_t structure will get overwritten since IFFT is computed in-place. Keep a copy of input->length and assign it back after ifft call.
    //This is done to avoid having to call bfp_complex_s32_init() on the input every frame
    uint32_t len = input->length;
    bfp_fft_pack_mono(input);
    bfp_s32_t *temp = bfp_fft_inverse_mono(input);
    memcpy(output, temp, sizeof(bfp_s32_t));

    input->length = len;
}

float_s32_t aec_calc_corr_factor(
        aec_state_t *state,
        unsigned ch) {
    // We need yhat[240:480-32] and y[240:480-32]
    int frame_window = 32;

    // y[240:480] is prev_y[0:240].
    bfp_s32_t y_subset;
    bfp_s32_init(&y_subset, state->shared_state->prev_y[ch].data, state->shared_state->prev_y[ch].exp, AEC_FRAME_ADVANCE-frame_window, 1);

    bfp_s32_t yhat_subset;
    bfp_s32_init(&yhat_subset, &state->y_hat[ch].data[AEC_FRAME_ADVANCE], state->y_hat[ch].exp, AEC_FRAME_ADVANCE-frame_window, 1);

    float_s32_t corr_factor = aec_priv_calc_corr_factor(&y_subset, &yhat_subset);
    return corr_factor;
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
    bfp_s32_init(&y_hat_subset, &state->y_hat[ch].data[AEC_FRAME_ADVANCE], state->y_hat[ch].exp, AEC_FRAME_ADVANCE, 1);

    //y[240:480] is prev_y[0:240]. Create a temporary bfp_s32_t to point to prev_y[0:240]
    bfp_s32_t temp;
    bfp_s32_init(&temp, state->shared_state->prev_y[ch].data, state->shared_state->prev_y[ch].exp, AEC_FRAME_ADVANCE, 1);

    aec_priv_calc_coherence(coh_mu_state_ptr, &temp, &y_hat_subset, &state->shared_state->config_params);
}

void aec_calc_output(
        aec_state_t *state,
        int32_t (*output)[AEC_FRAME_ADVANCE],
        unsigned ch)
{
    if(state == NULL) {
        return;
    }

    bfp_s32_t output_struct;
    if(output != NULL) {
        bfp_s32_init(&output_struct, &output[0][0], AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 0);
    }
    else {
        bfp_s32_init(&output_struct, NULL, AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 0);
    }
    bfp_s32_t *output_ptr = &output_struct;
    bfp_s32_t *overlap_ptr = &state->overlap[ch];
    bfp_s32_t *error_ptr = &state->error[ch];
    aec_priv_create_output(output_ptr, overlap_ptr, error_ptr);
}

void aec_calc_freq_domain_energy(
        float_s32_t *fd_energy,
        const bfp_complex_s32_t *input)
{
    int32_t DWORD_ALIGNED scratch_mem[AEC_PROC_FRAME_LENGTH/2 + 1];
    assert(input->length <= AEC_PROC_FRAME_LENGTH/2 + 1);
    bfp_s32_t scratch;
    bfp_s32_init(&scratch, scratch_mem, 0, input->length, 0);
    bfp_complex_s32_squared_mag(&scratch, input);

    float_s64_t sum64 = bfp_s32_sum(&scratch);
    *fd_energy = float_s64_to_float_s32(sum64);
}

void aec_calc_normalisation_spectrum(
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
    unsigned normdenom_apply_factor_of_2 = 0;
    aec_priv_calc_inv_X_energy(&state->inv_X_energy[ch], X_energy_ptr, sigma_XX_ptr, &state->shared_state->config_params, state->delta, is_shadow, normdenom_apply_factor_of_2);
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

    aec_priv_filter_adapt(state->H_hat[y_ch], state->X_fifo_1d, T_ptr, state->shared_state->num_x_channels, state->num_phases);
}

void aec_calc_T(
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

void aec_reset_state(aec_state_t *main_state, aec_state_t *shadow_state){
    aec_shared_state_t *shared_state = main_state->shared_state; 
    uint32_t y_channels = shared_state->num_y_channels;
    uint32_t x_channels = shared_state->num_x_channels;
    uint32_t main_phases = main_state->num_phases;
    uint32_t shadow_phases = shadow_state->num_phases;
    //Main H_hat
    for(int ch=0; ch<y_channels; ch++) {
        for(int ph=0; ph<x_channels*main_phases; ph++) {
            main_state->H_hat[ch][ph].exp = AEC_ZEROVAL_EXP;
            main_state->H_hat[ch][ph].hr = AEC_ZEROVAL_HR;
        }
    }
    //Shadow H_hat
    for(int ch=0; ch<y_channels; ch++) {
        for(int ph=0; ph<x_channels*shadow_phases; ph++) {
            shadow_state->H_hat[ch][ph].exp = AEC_ZEROVAL_EXP;
            shadow_state->H_hat[ch][ph].hr = AEC_ZEROVAL_HR;
        }
    }
    //X_fifo
    for(int ch=0; ch<x_channels; ch++) {
        for(int ph=0; ph<main_phases; ph++) {
            shared_state->X_fifo[ch][ph].exp = AEC_ZEROVAL_EXP;
            shared_state->X_fifo[ch][ph].hr = AEC_ZEROVAL_HR;
        }
    }
    //X_energy, sigma_XX
    for(int ch=0; ch<x_channels; ch++) {
        main_state->X_energy[ch].exp = AEC_ZEROVAL_EXP;
        main_state->X_energy[ch].hr = AEC_ZEROVAL_HR;
        shadow_state->X_energy[ch].exp = AEC_ZEROVAL_EXP;
        shadow_state->X_energy[ch].hr = AEC_ZEROVAL_HR;
        shared_state->sigma_XX[ch].exp = AEC_ZEROVAL_EXP;
        shared_state->sigma_XX[ch].hr = AEC_ZEROVAL_HR;
    }
}

uint32_t aec_detect_input_activity(const int32_t (*input_data)[AEC_FRAME_ADVANCE], float_s32_t active_threshold, int32_t num_channels) {
    /*abs_max_ref = abs(np.max(new_frame))
    return abs_max_ref > threshold*/
    bfp_s32_t ref, abs;
    int32_t scratch[AEC_FRAME_ADVANCE];
    bfp_s32_init(&abs, scratch, AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 0);
    for(int ch=0; ch<num_channels; ch++) {
        bfp_s32_init(&ref, (int32_t*)&input_data[ch][0], AEC_INPUT_EXP, AEC_FRAME_ADVANCE, 1);
        bfp_s32_abs(&abs, &ref);
        float_s32_t max = bfp_s32_max(&abs);
        if(float_s32_gt(max, active_threshold)) {
            return 1;
        }  
    }
    return 0;
}
