#include <limits.h>
#include "ic_low_level.h"

//Because we are going to heavily borrow from lib_aec..
#include "aec_defines.h"
#include "aec_api.h"
#include "aec_priv.h"


/// Sets up IC for processing a new frame
void ic_frame_init(
        ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE]){
    // y frame 
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        /* Create 512 samples frame */
        // Copy previous y samples
        memcpy(state->y_bfp[ch].data, state->prev_y_bfp[ch].data, (IC_FRAME_LENGTH-IC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy and apply delay to current y samples

        // memcpy(&state->y_bfp[ch].data[IC_FRAME_LENGTH-IC_FRAME_ADVANCE], y_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        for(unsigned i=0; i<IC_FRAME_ADVANCE; i++){
            unsigned input_delay_idx = state->y_delay_idx[ch];
            state->y_bfp[ch].data[IC_FRAME_LENGTH-IC_FRAME_ADVANCE+i] = state->y_input_delay[ch][input_delay_idx];
            state->y_input_delay[ch][input_delay_idx] = y_data[i];
            input_delay_idx++;
            if(input_delay_idx == IC_Y_CHANNEL_DELAY_SAMPS){
                input_delay_idx = 0;
            }
            state->y_delay_idx[ch] = input_delay_idx;
        }

        // Update exp just in case
        state->y_bfp[ch].exp = -31;
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
        state->prev_y_bfp[ch].exp = -31;
    }
    // x frame 
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        /* Create 512 samples frame */
        // Copy previous x samples
        memcpy(state->x_bfp[ch].data, state->prev_x_bfp[ch].data, (IC_FRAME_LENGTH-IC_FRAME_ADVANCE)*sizeof(int32_t));
        // Copy current x samples
        memcpy(&state->x_bfp[ch].data[IC_FRAME_LENGTH-IC_FRAME_ADVANCE], x_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        state->x_bfp[ch].exp = -31;
        // Update headroom
        bfp_s32_headroom(&state->x_bfp[ch]);

        /* Update previous samples */
        // Copy the last 32 samples to the beginning
        memcpy(state->prev_x_bfp[ch].data, &state->prev_x_bfp[ch].data[IC_FRAME_ADVANCE], (IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))*sizeof(int32_t));
        // Copy current frame to previous
        memcpy(&state->prev_x_bfp[ch].data[(IC_FRAME_LENGTH-(2*IC_FRAME_ADVANCE))], x_data, IC_FRAME_ADVANCE*sizeof(int32_t));
        // Update exp just in case
        state->prev_x_bfp[ch].exp = -31;
        // Update headroom
        bfp_s32_headroom(&state->prev_x_bfp[ch]);
    }

    //Initialise T
    //At the moment, there's only enough memory for storing IC_X_CHANNELS and not num_y_channels*num_x_channels worth of T.
    //So T calculation cannot be parallelised across Y channels
    //Reuse X memory for calculating T
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->T_bfp[ch], (complex_s32_t*)&state->x_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH, 0);
    }

    //set Y_hat memory to 0 since it will be used in bfp_complex_s32_macc operation in aec_l2_calc_Error_and_Y_hat()
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        state->Y_hat_bfp[ch].exp = -1024;
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
        const ic_config_params_t *conf){
    
    if(!length) {
        return;
    }
    bfp_s32_t input_chunk;
    bfp_s32_init(&input_chunk, &input->data[start_offset], input->exp, length, 0);
    input_chunk.hr = input->hr;
    float_s64_t dot64 = bfp_s32_dot(&input_chunk, &input_chunk);
    float_s32_t dot = float_s64_to_float_s32(dot64);
    *ema_energy = float_s32_ema(*ema_energy, dot, conf->core_conf.ema_alpha_q30);

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
    uint32_t sigma_xx_shift = state->config_params.core_conf.sigma_xx_shift;
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

    int32_t bypass_enabled = state->config_params.core_conf.bypass;
    aec_priv_calc_Error_and_Y_hat(Error_ptr, Y_hat_ptr, Y_ptr, X_fifo, H_hat, IC_X_CHANNELS, IC_FILTER_PHASES, bypass_enabled);
}


/// Window error. Overlap add to create IC output
void ic_create_output(
        ic_state_t *state,
        int32_t output[IC_FRAME_ADVANCE],
        unsigned ch){

    bfp_s32_t output_struct;
    bfp_s32_init(&output_struct, output, -31, IC_FRAME_ADVANCE, 0);
    bfp_s32_t *output_ptr = &output_struct;
    bfp_s32_t *overlap_ptr = &state->overlap_bfp[ch];
    bfp_s32_t *error_ptr = &state->error_bfp[ch];
    aec_config_params_t aec_conf; //Note unitinitialsed - this struct is not actually used by aec_priv_create_output

    aec_priv_create_output(output_ptr, overlap_ptr, error_ptr, &aec_conf);
    
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
    aec_conf.aec_core_conf.gamma_log2 = state->config_params.core_conf.gamma_log2;
    const unsigned disable_freq_smoothing = 0;
    aec_priv_calc_inv_X_energy(&state->inv_X_energy_bfp[ch], X_energy_ptr, sigma_XX_ptr, &aec_conf, state->delta, disable_freq_smoothing);
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
void ic_filter_adapt(
        ic_state_t *state){
    if(state == NULL) {
        return;
    }
    if(state->ic_adaption_controller_state.adaption_mode != IC_ADAPTION_ON) {
        return;
    }
    bfp_complex_s32_t *T_ptr = &state->T_bfp[0];
    int y_ch = 0;
    aec_priv_filter_adapt(state->H_hat_bfp[y_ch], state->X_fifo_1d_bfp, T_ptr, IC_X_CHANNELS, IC_FILTER_PHASES);
}


void ic_update_vad_history(ic_state_t *state, int32_t output[IC_FRAME_ADVANCE], uint8_t *vad_ptr){
    
    ic_adaption_controller_state_t *ad_state = &state->ic_adaption_controller_state;

    //Slide the window along
    for(int s = IC_FRAME_LENGTH - 1 - IC_FRAME_ADVANCE; s >= 0;s--){
        ad_state->vad_data_window[s + IC_FRAME_ADVANCE] = ad_state->vad_data_window[s];
    }

    //Add the new IC output in
    for(unsigned s=0; s<IC_FRAME_ADVANCE; s++){
        ad_state->vad_data_window[s] = output[s];
    }

    //From https://github.com/xmos/lib_audio_pipelines/blob/2f352c87a058c6f91c43ed4b69e77bd63db24ff4/lib_audio_pipelines/src/dsp/ap_stage_b.xc#L171
    //Keep VAD zero for first 12 frames
    //TODO - WHY? check with model!
    if(ad_state->vad_counter < 12){
        *vad_ptr = 0;
        (ad_state->vad_counter)++;
    }
}


void ic_adaption_controller(ic_state_t *state, uint8_t vad){

    ic_adaption_controller_state_t *ad_state = &state->ic_adaption_controller_state;

    float_s32_t r = {vad, -8}; //convert to float between 0 and 0.99609375

    ad_state->smoothed_voice_chance = float_s32_mul(ad_state->smoothed_voice_chance, ad_state->voice_chance_alpha);

    if(float_s32_gt(r, ad_state->smoothed_voice_chance)){
        ad_state->smoothed_voice_chance = r;
    }
    const float_s32_t one = {1, 0};
    const float_s32_t zero = {0, 0};

    float_s32_t mu = float_s32_sub(one, ad_state->smoothed_voice_chance);
    float_s32_t ratio = one;
    if(float_s32_gt(ad_state->input_energy, zero)){ //Protect against div by zero
        ratio = float_s32_div(ad_state->output_energy, ad_state->input_energy);
    }
 if (ad_state->enable_filter_instability_recovery){
        if(float_s32_gte(ratio, ad_state->out_to_in_ratio_limit)){
            ic_reset_filter(state);
        }
    }

    if(float_s32_gte(ratio, one)){
        //ratio clamps to one
        ratio = one;
    } else {
        ratio = float_s32_sqrt(ratio);
        ratio = float_s32_sqrt(ratio);
    }

    mu = float_s32_mul(mu, ratio);

    float_s32_t fast_ratio = one;
    if(float_s32_gt(ad_state->input_energy0, zero)){ //Protect against div by zero
        fast_ratio = float_s32_div(ad_state->output_energy0, ad_state->input_energy0);
    }
    if(float_s32_gt(fast_ratio, one)){
        state->ic_adaption_controller_state.leakage_alpha = state->ic_adaption_controller_state.instability_recovery_leakage_alpha;
        mu = zero;
    } else {
        state->ic_adaption_controller_state.leakage_alpha = one;
        // mu = mu;
    }

    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            state->mu[ych][xch] = mu;
        }
    } 
}

//TODO these are not used in lib_aec so not needed here?

// /// Unify bfp_complex_s32_t chunks into a single exponent and headroom
// void ic_l2_bfp_complex_s32_unify_exponent(
//         bfp_complex_s32_t *chunks,
//         int *final_exp,
//         int *final_hr,
//         const int *mapping,
//         int array_len,
//         int desired_index,
//         int min_headroom){

// }

// /// Unify bfp_s32_t chunks into a single exponent and headroom
// void ic_l2_bfp_s32_unify_exponent(
//         bfp_s32_t *chunks,
//         int *final_exp,
//         int *final_hr,
//         const int *mapping,
//         int array_len,
//         int desired_index,
//         int min_headroom){

// }

//Untested API TODO add unit tests
void ic_reset_filter(
        ic_state_t *state){
    
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_t *H_hat = state->H_hat_bfp[ch];
        aec_priv_reset_filter(H_hat, IC_X_CHANNELS, IC_FILTER_PHASES);
    }
}

//Untested API TODO add unit tests
//This mimics vtb_scale_complex_asm
void ic_apply_leakage(
        ic_state_t *state,
        unsigned y_ch){

        int32_t mant = state->ic_adaption_controller_state.leakage_alpha.mant;
        exponent_t exp = state->ic_adaption_controller_state.leakage_alpha.exp;
        float_s32_t leakage = {mant, exp};

        for(int ph=0; ph<IC_X_CHANNELS*IC_FILTER_PHASES; ph++){
            bfp_complex_s32_t *H_hat_ptr = &state->H_hat_bfp[y_ch][ph];
            bfp_complex_s32_real_scale(H_hat_ptr, H_hat_ptr, leakage); 
        }
}
