// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "ic_low_level.h"

// lib_ic heavily reuses functions from lib_aec currently
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"

// Delay y input w.r.t. x input
void ic_delay_y_input(ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE]){
    // Run through delay line
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        unsigned input_delay_idx = state->y_delay_idx[ch];
        for(unsigned i=0; i<IC_FRAME_ADVANCE; i++){
            int32_t tmp = state->y_input_delay[ch][input_delay_idx];
            state->y_input_delay[ch][input_delay_idx] = y_data[i];
            y_data[i] = tmp;
            input_delay_idx++;
            if(input_delay_idx == IC_Y_CHANNEL_DELAY_SAMPS){
                input_delay_idx = 0;
            }
        }
        state->y_delay_idx[ch] = input_delay_idx;
    }
}

// Sets up IC for processing a new frame
void ic_frame_init(
        ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE]){
    
    const exponent_t q0_31_exp = -31;
    // y frame 
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        /* Create 512 samples frame */
        // Copy previous y samples
        memcpy(state->y_bfp[ch].data, state->prev_y_bfp[ch].data, (IC_FRAME_LENGTH-IC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy and apply delay to current y samples
        memcpy(&state->y_bfp[ch].data[IC_FRAME_LENGTH-IC_FRAME_ADVANCE], y_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        const exponent_t q0_31_exp = -31;
        state->y_bfp[ch].exp = q0_31_exp;
        // Update headroom
        bfp_s32_headroom(&state->y_bfp[ch]);

        /* Update previous samples */
        // Save a copy of the first 240 samples of prev_y_bfp that are about to get overwritten, in case it's needed in ic_reset_filter() to recreate the original 512 samples time-domain frame.
        memcpy(&state->y_prev_samples_copy[ch][0], &state->prev_y_bfp[ch].data[0], IC_FRAME_ADVANCE*sizeof(int32_t));
        // Copy the last 32 samples to the beginning
        memcpy(state->prev_y_bfp[ch].data, &state->prev_y_bfp[ch].data[IC_FRAME_ADVANCE], (IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))*sizeof(int32_t));
        // Copy current frame to previous
        memcpy(&state->prev_y_bfp[ch].data[(IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))], y_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update headroom
        bfp_s32_headroom(&state->prev_y_bfp[ch]);
        // Update exp just in case
        state->prev_y_bfp[ch].exp = q0_31_exp;
    }
    // x frame 
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        /* Create 512 samples frame */
        // Copy previous x samples
        memcpy(state->x_bfp[ch].data, state->prev_x_bfp[ch].data, (IC_FRAME_LENGTH-IC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy current x samples
        memcpy(&state->x_bfp[ch].data[IC_FRAME_LENGTH-IC_FRAME_ADVANCE], x_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        state->x_bfp[ch].exp = q0_31_exp;
        // Update headroom
        bfp_s32_headroom(&state->x_bfp[ch]);

        /* Update previous samples */
        // Copy the last 32 samples to the beginning
        memcpy(state->prev_x_bfp[ch].data, &state->prev_x_bfp[ch].data[IC_FRAME_ADVANCE], (IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))*sizeof(int32_t));
        // Copy current frame to previous
        memcpy(&state->prev_x_bfp[ch].data[(IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))], x_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        state->prev_x_bfp[ch].exp = q0_31_exp;
        // Update headroom
        bfp_s32_headroom(&state->prev_x_bfp[ch]);
    }

    // Initialise T
    // At the moment, there's only enough memory for storing IC_X_CHANNELS and not num_y_channels*num_x_channels worth of T.
    // Reuse X memory for calculating T
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->T_bfp[ch], (complex_s32_t*)&state->x_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH, 0);
    }

    // Set Y_hat memory to 0 since it will be used in bfp_complex_s32_macc operation in aec_l2_calc_Error_and_Y_hat()
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        const exponent_t zero_exp = -1024;
        state->Y_hat_bfp[ch].exp = zero_exp;
        state->Y_hat_bfp[ch].hr = 0;
        memset(&state->Y_hat_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH*sizeof(complex_s32_t));
    }
}

// Calculate average energy in time domain
void ic_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const uq2_30 alpha){
    
    if(!length) {
        return;
    }
    bfp_s32_t input_chunk;
    bfp_s32_init(&input_chunk, &input->data[start_offset], input->exp, length, 0);
    input_chunk.hr = input->hr;
    float_s64_t dot64 = bfp_s32_dot(&input_chunk, &input_chunk);
    float_s32_t dot = float_s64_to_float_s32(dot64);
    *ema_energy = float_s32_ema(*ema_energy, dot, alpha);
}

// FFT single channel real input
void ic_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input){

    aec_forward_fft(output, input);

}

// Real IFFT to single channel input data
void ic_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        ){

    aec_inverse_fft(output, input);

}

// Calculate X energy
void ic_update_X_energy(
        ic_state_t *state,
        unsigned ch,
        unsigned recalc_bin){
    bfp_s32_t *X_energy_ptr = &state->X_energy_bfp[ch];
    bfp_complex_s32_t *X_ptr = &state->X_bfp[ch];
    float_s32_t *max_X_energy_ptr = &state->max_X_energy[ch];
    aec_priv_update_total_X_energy(X_energy_ptr, max_X_energy_ptr, &state->X_fifo_bfp[ch][0], X_ptr, IC_FILTER_PHASES, recalc_bin);
}

// Update X-fifo with the newest X data. Calculate sigmaXX
void ic_update_X_fifo_and_calc_sigmaXX(
        ic_state_t *state,
        unsigned ch){

    bfp_s32_t *sigma_XX_ptr = &state->sigma_XX_bfp[ch];
    bfp_complex_s32_t *X_ptr = &state->X_bfp[ch];
    uint32_t sigma_xx_shift = state->config_params.sigma_xx_shift;
    float_s32_t *sum_X_energy_ptr = &state->sum_X_energy[ch];
    aec_priv_update_X_fifo_and_calc_sigmaXX(&state->X_fifo_bfp[ch][0], sigma_XX_ptr, sum_X_energy_ptr, X_ptr, IC_FILTER_PHASES, sigma_xx_shift);

}

// Update the 1d form of X-fifo pointers
void ic_update_X_fifo_1d(
        ic_state_t *state){
    unsigned count = 0;
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        for(unsigned ph=0; ph<IC_FILTER_PHASES; ph++) {
            state->X_fifo_1d_bfp[count] = state->X_fifo_bfp[ch][ph];
            count += 1;
        }
    }
}

// Calculate filter Error and Y_hat
void ic_calc_Error_and_Y_hat(
        ic_state_t *state,
        unsigned ch){
    bfp_complex_s32_t *Y_ptr = &state->Y_bfp[ch];
    bfp_complex_s32_t *Y_hat_ptr = &state->Y_hat_bfp[ch];
    bfp_complex_s32_t *Error_ptr = &state->Error_bfp[ch];
    bfp_complex_s32_t *X_fifo = state->X_fifo_1d_bfp;
    bfp_complex_s32_t *H_hat = state->H_hat_bfp[ch];

    int32_t bypass_enabled = state->config_params.bypass;
    aec_priv_calc_Error_and_Y_hat(Error_ptr, Y_hat_ptr, Y_ptr, X_fifo, H_hat, IC_X_CHANNELS, IC_FILTER_PHASES, bypass_enabled);
}

// Window error. Overlap add to create IC output
void ic_create_output(
        ic_state_t *state,
        int32_t output[IC_FRAME_ADVANCE],
        unsigned ch){

    bfp_s32_t output_struct;
    const exponent_t q0_31_exp = -31;
    bfp_s32_init(&output_struct, output, q0_31_exp, IC_FRAME_ADVANCE, 0);
    bfp_s32_t *output_ptr = &output_struct;
    bfp_s32_t *overlap_ptr = &state->overlap_bfp[ch];
    bfp_s32_t *error_ptr = &state->error_bfp[ch];

    aec_priv_create_output(output_ptr, overlap_ptr, error_ptr);
    
}

// Calculate inverse X-energy
void ic_calc_inv_X_energy(
        ic_state_t *state,
        unsigned ch){

    //frequency smoothing
    //calc inverse energy
    bfp_s32_t *sigma_XX_ptr = &state->sigma_XX_bfp[ch];
    bfp_s32_t *X_energy_ptr = &state->X_energy_bfp[ch];

    //Make a copy of aec_conf so we can pass the right type
    aec_config_params_t aec_conf; //Only gamma_log2 is accessed in aec_priv_calc_inv_X_energy_denom
    aec_conf.aec_core_conf.gamma_log2 = state->config_params.gamma_log2;
    const unsigned disable_freq_smoothing = 0;
    const unsigned normdenom_apply_factor_of_2 = 0;
    aec_priv_calc_inv_X_energy(&state->inv_X_energy_bfp[ch], X_energy_ptr, sigma_XX_ptr, &aec_conf, state->config_params.delta, disable_freq_smoothing, normdenom_apply_factor_of_2);
}

// Calculate T (mu * inv_X_energy * Error)
void ic_compute_T(
        ic_state_t *state,
        unsigned y_ch,
        unsigned x_ch){

    bfp_complex_s32_t *T_ptr = &state->T_bfp[x_ch]; // We reuse the same memory as X to store T
    bfp_complex_s32_t *Error_ptr = &state->Error_bfp[y_ch];
    bfp_s32_t *inv_X_energy_ptr = &state->inv_X_energy_bfp[x_ch];
    float_s32_t mu = state->mu[y_ch][x_ch];

    aec_priv_compute_T(T_ptr, Error_ptr, inv_X_energy_ptr, mu);
}

// Adapt H_hat
void ic_filter_adapt(ic_state_t *state){
    if((state->ic_adaption_controller_state.adaption_controller_config.enable_adaption == 0) ||
       state->config_params.bypass) {
        return;
    }
    bfp_complex_s32_t *T_ptr = &state->T_bfp[0];
    int y_ch = 0;
    aec_priv_filter_adapt(state->H_hat_bfp[y_ch], state->X_fifo_1d_bfp, T_ptr, IC_X_CHANNELS, IC_FILTER_PHASES);
}

// Arithmetic shift for a signed int32_t
static inline int32_t ashr32(int32_t x, right_shift_t shr)
{
  if(shr >= 0)
    return x >> shr;
  
  int64_t tmp = ((int64_t)x) << -shr;

  if(tmp > INT32_MAX)       return INT32_MAX;
  else if(tmp < INT32_MIN)  return INT32_MIN;
  else                      return tmp;
}

// Temporary implementation of float_s32_add which handles 32 bit shifts
float_s32_t float_s32_add_fix(
    const float_s32_t x,
    const float_s32_t y)
{
  float_s32_t res;

  const headroom_t x_hr = HR_S32(x.mant);
  const headroom_t y_hr = HR_S32(y.mant);

  const exponent_t x_min_exp = x.exp - x_hr;
  const exponent_t y_min_exp = y.exp - y_hr;

  res.exp = MAX(x_min_exp, y_min_exp) + 1;

  const right_shift_t x_shr = res.exp - x.exp;
  const right_shift_t y_shr = res.exp - y.exp;
  
  int32_t x_mant = (x_shr >= 32) ? 0 : ashr32(x.mant, x_shr);
  int32_t y_mant = (y_shr >= 32) ? 0 : ashr32(y.mant, y_shr);

  res.mant = x_mant + y_mant;

  return res;
}

// Calculates fast ratio
void ic_calc_fast_ratio(ic_adaption_controller_state_t * ad_state){
    const float_s32_t delta = {7037, -46}; // ~ 0.0000000001 from Python model
    float_s32_t denom = float_s32_add_fix(ad_state->input_energy, delta);
    ad_state->fast_ratio = float_s32_div(ad_state->output_energy, denom);
}

// Sets mu
void ic_set_mu(ic_state_t * state, float_s32_t mu){
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            state->mu[ych][xch] = mu;
        }
    }
}

// VNR based mu control system
void ic_mu_control_system(ic_state_t * state, float_s32_t vnr){
    ic_adaption_controller_state_t *ad_state = &state->ic_adaption_controller_state;
    ic_adaption_controller_config_t *ad_config = &state->ic_adaption_controller_state.adaption_controller_config;
    
    const float_s32_t one = f32_to_float_s32(1.0);
    const float_s32_t zero = f32_to_float_s32(0.0);

    if(ad_config->adaption_config == IC_ADAPTION_FORCE_ON){
        ad_state->control_flag = FORCE_ADAPT;
        return;
    }
    if(ad_config->adaption_config == IC_ADAPTION_FORCE_OFF){
        ic_set_mu(state, zero);
        state->leakage_alpha = one;
        ad_state->control_flag = FORCE_HOLD;
        return;
    }

    if(float_s32_gte(vnr, ad_config->input_vnr_threshold)){
        ic_set_mu(state, zero);
        if(float_s32_gt(vnr, ad_config->input_vnr_threshold_high)){
            state->leakage_alpha = ad_config->high_input_vnr_hold_leakage_alpha;
        }
        else{
            state->leakage_alpha = one;
        }
        ad_state->control_flag = HOLD;
        ad_state->adapt_counter = 0;
    }
    else{
        if((ad_state->adapt_counter <= ad_config->adapt_counter_limit)||(float_s32_gte(ad_config->input_vnr_threshold_low, vnr))){
            ic_set_mu(state, one);
            ad_state->control_flag = ADAPT;
        }
        else{
            ic_set_mu(state, f32_to_float_s32(0.1));
            ad_state->control_flag = ADAPT_SLOW;
        }
        state->leakage_alpha = one;
        ad_state->adapt_counter++;
    }

    if(float_s32_gt(ad_state->fast_ratio, ad_config->fast_ratio_threshold)){
        ic_set_mu(state, f32_to_float_s32(0.9));
        state->leakage_alpha = ad_config->instability_recovery_leakage_alpha;
        ad_state->control_flag = UNSTABLE;
    }
    //printf("MU: %ld %d\n", state->mu[0][0].mant, state->mu[0][0].exp);
}

// Reset adaptive components and output an unprocessed frame
void ic_reset_filter(ic_state_t *state, int32_t output[IC_FRAME_ADVANCE]){
    
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_t *H_hat = state->H_hat_bfp[ch];
        aec_priv_reset_filter(H_hat, IC_X_CHANNELS, IC_FILTER_PHASES);
    }
    const exponent_t zero_exp = -1024;
    for(unsigned ch = 0; ch < IC_X_CHANNELS; ch ++){
        bfp_s32_set(&state->sigma_XX_bfp[ch], 0, zero_exp);
    }
    // Getting unproccessed y frame from state->y_prev_samples_copy[ch] and state->prev_y_bfp[ch].data 
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        int32_t DWORD_ALIGNED buff[IC_FRAME_LENGTH];
        memcpy(&buff[0], &state->y_prev_samples_copy[ch][0], IC_FRAME_ADVANCE*sizeof(int32_t));
        memcpy(&buff[IC_FRAME_ADVANCE], &state->prev_y_bfp[ch].data[0], (IC_FRAME_LENGTH - IC_FRAME_ADVANCE)*sizeof(int32_t));
        const exponent_t init_exp = -31;
        bfp_s32_t y, out;
        bfp_s32_init(&y, buff, init_exp, IC_FRAME_LENGTH, 1);
        bfp_s32_init(&out, output, init_exp, IC_FRAME_ADVANCE, 0);
        aec_priv_create_output(&out, &state->overlap_bfp[ch], &y);
    }
}

// This allows the filter to forget some of its training
void ic_apply_leakage(
    ic_state_t *state,
    unsigned y_ch){

    ic_adaption_controller_config_t *ad_config = &state->ic_adaption_controller_state.adaption_controller_config;

    if((ad_config->enable_adaption == 0) ||
       state->config_params.bypass) {
        return;
    }

    for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
        bfp_complex_s32_t *H_hat_ptr = &state->H_hat_bfp[y_ch][ph];
        bfp_complex_s32_real_scale(H_hat_ptr, H_hat_ptr, state->leakage_alpha); 
    }
}
