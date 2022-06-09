// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "ic_low_level.h"
#include <limits.h>

//lib_ic heavily reuses functions from lib_aec currently
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"

///Delay y input w.r.t. x input
void ic_delay_y_input(ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE]){
    //Run through delay line
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

/// Sets up IC for processing a new frame
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

    //Initialise T
    //At the moment, there's only enough memory for storing IC_X_CHANNELS and not num_y_channels*num_x_channels worth of T.
    //Reuse X memory for calculating T
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->T_bfp[ch], (complex_s32_t*)&state->x_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH, 0);
    }

    //set Y_hat memory to 0 since it will be used in bfp_complex_s32_macc operation in aec_l2_calc_Error_and_Y_hat()
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        const exponent_t zero_exp = -1024;
        state->Y_hat_bfp[ch].exp = zero_exp;
        state->Y_hat_bfp[ch].hr = 0;
        memset(&state->Y_hat_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH*sizeof(complex_s32_t));
    }
}

/// Calculate average energy in time domain
void ic_update_td_ema_energy(
        float_s32_t *ema_energy,
        const bfp_s32_t *input,
        unsigned start_offset,
        unsigned length,
        const fixed_s32_t alpha){
    
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

/// FFT single channel real input
void ic_fft(
        bfp_complex_s32_t *output,
        bfp_s32_t *input){

    aec_forward_fft(output, input);

}

/// Real IFFT to single channel input data
void ic_ifft(
        bfp_s32_t *output,
        bfp_complex_s32_t *input
        ){

    aec_inverse_fft(output, input);

}

/// Calculate X energy
void ic_update_X_energy(
        ic_state_t *state,
        unsigned ch,
        unsigned recalc_bin){
    bfp_s32_t *X_energy_ptr = &state->X_energy_bfp[ch];
    bfp_complex_s32_t *X_ptr = &state->X_bfp[ch];
    float_s32_t *max_X_energy_ptr = &state->max_X_energy[ch];
    aec_priv_update_total_X_energy(X_energy_ptr, max_X_energy_ptr, &state->X_fifo_bfp[ch][0], X_ptr, IC_FILTER_PHASES, recalc_bin);
}

/// Update X-fifo with the newest X data. Calculate sigmaXX
void ic_update_X_fifo_and_calc_sigmaXX(
        ic_state_t *state,
        unsigned ch){

    bfp_s32_t *sigma_XX_ptr = &state->sigma_XX_bfp[ch];
    bfp_complex_s32_t *X_ptr = &state->X_bfp[ch];
    uint32_t sigma_xx_shift = state->config_params.sigma_xx_shift;
    float_s32_t *sum_X_energy_ptr = &state->sum_X_energy[ch];
    aec_priv_update_X_fifo_and_calc_sigmaXX(&state->X_fifo_bfp[ch][0], sigma_XX_ptr, sum_X_energy_ptr, X_ptr, IC_FILTER_PHASES, sigma_xx_shift);

}

/// Update the 1d form of X-fifo pointers
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

/// Calculate filter Error and Y_hat
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

/// Window error. Overlap add to create IC output
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

/// Calculate inverse X-energy
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

/// Calculate T (mu * inv_X_energy * Error)
void ic_compute_T(
        ic_state_t *state,
        unsigned y_ch,
        unsigned x_ch){

    bfp_complex_s32_t *T_ptr = &state->T_bfp[x_ch]; //We reuse the same memory as X to store T
    bfp_complex_s32_t *Error_ptr = &state->Error_bfp[y_ch];
    bfp_s32_t *inv_X_energy_ptr = &state->inv_X_energy_bfp[x_ch];
    float_s32_t mu = state->mu[y_ch][x_ch];

    aec_priv_compute_T(T_ptr, Error_ptr, inv_X_energy_ptr, mu);
}

/// Adapt H_hat
void ic_filter_adapt(ic_state_t *state){
    if((state->ic_adaption_controller_state.adaption_controller_config.enable_adaption == 0) ||
       state->config_params.bypass) {
        return;
    }
    bfp_complex_s32_t *T_ptr = &state->T_bfp[0];
    int y_ch = 0;
    aec_priv_filter_adapt(state->H_hat_bfp[y_ch], state->X_fifo_1d_bfp, T_ptr, IC_X_CHANNELS, IC_FILTER_PHASES);
}

static inline int32_t ashr32(int32_t x, right_shift_t shr)
{
  if(shr >= 0)
    return x >> shr;
  
  int64_t tmp = ((int64_t)x) << -shr;

  if(tmp > INT32_MAX)       return INT32_MAX;
  else if(tmp < INT32_MIN)  return INT32_MIN;
  else                      return tmp;
}

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

//Python port
void ic_adaption_controller(ic_state_t *state, uint8_t vad){
    ic_adaption_controller_state_t *ad_state = &state->ic_adaption_controller_state;
    ic_adaption_controller_config_t *ad_config = &state->ic_adaption_controller_state.adaption_controller_config;

    if(!ad_config->enable_adaption_controller){ //skip this function if adaption controller not enabled
        return;
    }

    const float_s32_t one = {1, 0};
    const float_s32_t zero = {0, 0};
    //const float_s32_t delta = {1100, -40}; //1100 * 2**-40 = 0.000000001 (from stage_b.py)
    const float_s32_t delta = {352, -45}; //352 * 2**-45 ~ 0.00000000001

    exponent_t q0_8_exp = -8; 
    float_s32_t vad_float = {vad, q0_8_exp}; //convert to float between 0 and 0.99609375

    //self.smoothed_voice_chance = self.voice_chance_alpha*self.smoothed_voice_chance
    //self.smoothed_voice_chance = max(self.smoothed_voice_chance, vad_result)
    ad_state->smoothed_voice_chance = float_s32_mul(ad_state->smoothed_voice_chance, ad_config->voice_chance_alpha);
    if(float_s32_gt(vad_float, ad_state->smoothed_voice_chance)){
        ad_state->smoothed_voice_chance = vad_float;
    }

    //noise_mu = 1.0 - self.smoothed_voice_chance
    float_s32_t noise_mu = float_s32_sub(one, ad_state->smoothed_voice_chance);

    //noise_mu = noise_mu * min(1.0, np.sqrt(np.sqrt(self.output_energy/(self.input_energy + 0.000000001))))
    float_s32_t input_plus_delta = float_s32_add_fix(ad_state->input_energy_slow, delta);
    float_s32_t ratio = float_s32_div(ad_state->output_energy_slow, input_plus_delta);
    ratio = float_s32_sqrt(ratio);
    ratio = float_s32_sqrt(ratio);
    if(float_s32_gt(one, ratio)){ 
        ratio = one;
    }
    noise_mu = float_s32_mul(noise_mu, ratio);

    // THIS IS NOT IN PYTHON - Included as safety feature
    if(ad_config->enable_filter_instability_recovery){
        if(float_s32_gte(ratio, ad_config->out_to_in_ratio_limit)){
            ic_reset_filter(state);
        }
    }

    //fast_ratio = self.output_energy0 / (self.input_energy0 + 0.000000001)
    input_plus_delta = float_s32_add_fix(ad_state->input_energy_fast, delta);
    float_s32_t fast_ratio = float_s32_div(ad_state->output_energy_fast, input_plus_delta);

    // if fast_ratio > 1.0:
    //     self.ifc.set_leakage(0.995)
    //     self.ifc.set_mu(0.0)
    float_s32_t mu = zero;
    if(float_s32_gt(fast_ratio, one)){
        ad_config->leakage_alpha = ad_config->instability_recovery_leakage_alpha;//in ic_defines.h
        mu = zero;
    } 
    // else:
        // self.ifc.set_leakage (1.0)
        // self.ifc.set_mu(noise_mu)
    else {
        ad_config->leakage_alpha = one;
        mu = noise_mu;
    }

    //Now copy this into the actual state structure
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            state->mu[ych][xch] = mu;
        }
    } 
}


//Clear down filter to init state
void ic_reset_filter(ic_state_t *state){
    
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_t *H_hat = state->H_hat_bfp[ch];
        aec_priv_reset_filter(H_hat, IC_X_CHANNELS, IC_FILTER_PHASES);
    }
}

//This allows the filter to forget some of its training
void ic_apply_leakage(
    ic_state_t *state,
    unsigned y_ch){
    ic_adaption_controller_config_t *ad_config = &state->ic_adaption_controller_state.adaption_controller_config;


    if((ad_config->enable_adaption == 0) ||
       state->config_params.bypass) {
        return;
    }

    int32_t mant = ad_config->leakage_alpha.mant;
    exponent_t exp = ad_config->leakage_alpha.exp;
    float_s32_t leakage = {mant, exp};

    for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
        bfp_complex_s32_t *H_hat_ptr = &state->H_hat_bfp[y_ch][ph];
        bfp_complex_s32_real_scale(H_hat_ptr, H_hat_ptr, leakage); 
    }
}

void ic_priv_calc_vnr_pred(
    ic_vnr_pred_state_t *vnr_state,
    const bfp_complex_s32_t *Y,
    const bfp_complex_s32_t *Error
    )
{
    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH * VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_state->feature_state[0], &feature_patch, feature_patch_data, Y);
    float_s32_t ie_output;
    vnr_inference(&vnr_state->inference_state, &ie_output, &feature_patch);
    vnr_state->input_vnr_pred = float_s32_ema(vnr_state->input_vnr_pred, ie_output, vnr_state->pred_alpha_q30); 
    
    vnr_extract_features(&vnr_state->feature_state[1], &feature_patch, feature_patch_data, Error);
    vnr_inference(&vnr_state->inference_state, &ie_output, &feature_patch);
    vnr_state->output_vnr_pred = float_s32_ema(vnr_state->output_vnr_pred, ie_output, vnr_state->pred_alpha_q30);
}
