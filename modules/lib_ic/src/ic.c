// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "ic_low_level.h"
#include "xmath/xmath.h"

// For use when dumping variables for debug
void ic_dump_var_2d(ic_state_t *state);
void ic_dump_var_3d(ic_state_t *state);

static int32_t ic_init_vnr_pred_state(vnr_pred_state_t *vnr_pred_state){
    vnr_feature_state_init(&vnr_pred_state->feature_state[0]);
    vnr_feature_state_init(&vnr_pred_state->feature_state[1]);

    int32_t ret = vnr_inference_init();
    vnr_pred_state->pred_alpha_q30 = Q30(IC_INIT_VNR_PRED_ALPHA);
    vnr_pred_state->input_vnr_pred = f32_to_float_s32(IC_INIT_INPUT_VNR_PRED);
    vnr_pred_state->output_vnr_pred = f32_to_float_s32(IC_INIT_OUTPUT_VNR_PRED);
    return ret;
}

static void ic_init_config(ic_config_params_t *config){
    config->sigma_xx_shift = IC_INIT_SIGMA_XX_SHIFT;
    config->gamma_log2 = IC_INIT_GAMMA_LOG2;
    config->ema_alpha_q30 = Q30(IC_INIT_EMA_ALPHA);
    config->bypass = 0;
    config->delta = f64_to_float_s32(IC_INIT_DELTA);

}

static void ic_init_adaption_controller_config(ic_adaption_controller_config_t *ad_config){

    ad_config->energy_alpha_q30 = Q30(IC_INIT_ENERGY_ALPHA);

    ad_config->fast_ratio_threshold = f64_to_float_s32(IC_INIT_FAST_RATIO_THRESHOLD);
    ad_config->high_input_vnr_hold_leakage_alpha = f64_to_float_s32(IC_INIT_HIGH_INPUT_VNR_HOLD_LEAKAGE_ALPHA);
    ad_config->instability_recovery_leakage_alpha = f64_to_float_s32(IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA);

    ad_config->input_vnr_threshold = f64_to_float_s32(IC_INIT_INPUT_VNR_THRESHOLD);
    ad_config->input_vnr_threshold_high = f64_to_float_s32(IC_INIT_INPUT_VNR_THRESHOLD_HIGH);
    ad_config->input_vnr_threshold_low = f64_to_float_s32(IC_INIT_INPUT_VNR_THRESHOLD_LOW);

    ad_config->adapt_counter_limit = IC_INIT_ADAPT_COUNTER_LIMIT;

    ad_config->enable_adaption = 1;
    ad_config->adaption_config = IC_ADAPTION_AUTO;
}

static void ic_init_adaption_controller(ic_adaption_controller_state_t *ad_state){

    const float_s32_t zero = {0, 0};

    ad_state->input_energy = zero;

    ad_state->input_energy = zero;

    ad_state->fast_ratio = zero;

    ad_state->adapt_counter = 0;

    ad_state->control_flag = ADAPT;

    ic_init_adaption_controller_config(&ad_state->adaption_controller_config);
}

int32_t ic_init(ic_state_t *state){
    memset(state, 0, sizeof(ic_state_t));
    const exponent_t zero_exp = -1024;
    
    // H_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        for(unsigned ph=0; ph<(IC_X_CHANNELS * IC_FILTER_PHASES); ph++) {
            bfp_complex_s32_init(&state->H_hat_bfp[ch][ph], state->H_hat[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
        }
    }
    // X_fifo
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        for(unsigned ph=0; ph<IC_FILTER_PHASES; ph++) {
            bfp_complex_s32_init(&state->X_fifo_bfp[ch][ph], state->X_fifo[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
            bfp_complex_s32_init(&state->X_fifo_1d_bfp[ch * IC_FILTER_PHASES + ph], state->X_fifo[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
        }
    }
    // Initialise Error
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Error_bfp[ch], state->Error[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
        bfp_s32_init(&state->error_bfp[ch], (int32_t *)state->Error[ch], zero_exp, IC_FRAME_LENGTH, 0);
    }
    // Initiaise Y_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Y_hat_bfp[ch], state->Y_hat[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    // X_energy 
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->X_energy_bfp[ch], state->X_energy[ch], zero_exp, IC_FD_FRAME_LENGTH, 0); 
    }
    state->X_energy_recalc_bin = 0;

    // sigma_XX
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->sigma_XX_bfp[ch], state->sigma_XX[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    // inv_X_energy
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->inv_X_energy_bfp[ch], state->inv_X_energy[ch], zero_exp, IC_FD_FRAME_LENGTH, 0); 
    }

    // overlap
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->overlap_bfp[ch], state->overlap[ch], zero_exp, IC_FRAME_OVERLAP, 0);
    }

    // Y, note in-place with y
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->y_bfp[ch], state->y[ch], zero_exp, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->Y_bfp[ch], (complex_s32_t*)state->y[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
        state->y_delay_idx[ch] = 0; //init delay index 
    }
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_y_bfp[ch], state->y_prev_samples[ch], zero_exp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    // X, note in-place with x
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->x_bfp[ch], state->x[ch], zero_exp, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->X_bfp[ch], (complex_s32_t*)state->x[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_x_bfp[ch], state->x_prev_samples[ch], zero_exp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    // Reuse X memory for calculating T. Note we re-initialise T_bfp in ic_frame_init()
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->T_bfp[ch], (complex_s32_t*)&state->x_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH, 0);
    }

    // mu
    for(unsigned ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(unsigned xch=0; xch<IC_X_CHANNELS; xch++) {
            state->mu[ych][xch] = f64_to_float_s32(IC_INIT_MU);
        }
    }

    state->leakage_alpha = f64_to_float_s32(IC_INIT_LEAKAGE_ALPHA);

    // Initialise ic core config params and adaption controller
    ic_init_config(&state->config_params);
    ic_init_adaption_controller(&state->ic_adaption_controller_state);
    int32_t ret = ic_init_vnr_pred_state(&state->vnr_pred_state);
    return ret;
}

void ic_filter(
        ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE],
        int32_t output[IC_FRAME_ADVANCE])
{
    if(state == NULL) {
        return;
    }
    ic_adaption_controller_state_t *ad_state = &state->ic_adaption_controller_state;
    ic_adaption_controller_config_t *ad_config = &state->ic_adaption_controller_state.adaption_controller_config;

    // Delay y channel, necessary for operation of adaptive filter
    ic_delay_y_input(state, y_data);

    bfp_s32_t y_bfp_test;
    bfp_s32_init(&y_bfp_test, y_data, -31, IC_FRAME_ADVANCE, 1); 
    // Calculate input td ema energies
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_update_td_ema_energy(&ad_state->input_energy, &y_bfp_test, 0, IC_FRAME_ADVANCE, ad_config->energy_alpha_q30);
    }

    // Build a time domain frame of IC_FRAME_LENGTH from IC_FRAME_ADVANCE new samples
    ic_frame_init(state, y_data, x_data);


    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Y_bfp[ch], &state->y_bfp[ch]);
    }

    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_fft(&state->X_bfp[ch], &state->x_bfp[ch]);
    }

    // Update X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_energy(state, ch, state->X_energy_recalc_bin);
    }

    state->X_energy_recalc_bin += 1;
    if(state->X_energy_recalc_bin == IC_FD_FRAME_LENGTH) {
        state->X_energy_recalc_bin = 0;
    }

    // Update X_fifo with the new X and calcualate sigma_XX
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_fifo_and_calc_sigmaXX(state, ch);
    }

    // Update the 1 dimensional bfp structs that are also used to access X_fifo
    ic_update_X_fifo_1d(state);

    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_calc_Error_and_Y_hat(state, ch);
    }

    // IFFT Error (output)
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_ifft(&state->error_bfp[ch], &state->Error_bfp[ch]);
    }

    // Note the model supports noise minimisation but we have not ported this
    // as it has not been found to aid ASR performance

    // Window error. Calculate output
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_create_output(state, output, ch);
    }

    // Calculate output td ema energies
    /*ic_update_td_ema_energy(&ad_state->output_energy, &state->error_bfp[0], IC_FRAME_LENGTH - IC_FRAME_ADVANCE,
                            IC_FRAME_ADVANCE, ad_config->energy_alpha_q30);*/

    bfp_s32_t output_bfp_test;
    bfp_s32_init(&output_bfp_test, output, -31, IC_FRAME_ADVANCE, 1); 
    // Calculate input td ema energies
    ic_update_td_ema_energy(&ad_state->output_energy, &output_bfp_test, 0, IC_FRAME_ADVANCE, ad_config->energy_alpha_q30);

    // error -> Error FFT
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Error_bfp[ch], &state->error_bfp[ch]);
    }

    ic_calc_fast_ratio(ad_state);

    if((float_s32_gt(ad_state->fast_ratio, ad_config->fast_ratio_threshold))&&(ad_config->adaption_config == IC_ADAPTION_AUTO)){
        ic_reset_filter(state, output);
    }
}


void ic_calc_vnr_pred(
	ic_state_t * ic_state,
	float_s32_t * input_vnr_pred,
	float_s32_t * output_vnr_pred){

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH * VNR_MEL_FILTERS];
    vnr_extract_features(&ic_state->vnr_pred_state.feature_state[0], &feature_patch, feature_patch_data, &ic_state->Y_bfp[0]);
    float_s32_t ie_output;
    vnr_inference(&ie_output, &feature_patch);
    ic_state->vnr_pred_state.input_vnr_pred = float_s32_ema(ic_state->vnr_pred_state.input_vnr_pred, ie_output, ic_state->vnr_pred_state.pred_alpha_q30);
    *input_vnr_pred = ic_state->vnr_pred_state.input_vnr_pred;

    vnr_extract_features(&ic_state->vnr_pred_state.feature_state[1], &feature_patch, feature_patch_data, &ic_state->Error_bfp[0]);
    vnr_inference(&ie_output, &feature_patch);
    ic_state->vnr_pred_state.output_vnr_pred = float_s32_ema(ic_state->vnr_pred_state.output_vnr_pred, ie_output, ic_state->vnr_pred_state.pred_alpha_q30);
    *output_vnr_pred = ic_state->vnr_pred_state.output_vnr_pred;
}

void ic_adapt(
        ic_state_t *state,
        float_s32_t vnr){

    if(state == NULL) {
        return;
    }
   
    // Calculate leakage and mu for adaption
    ic_mu_control_system(state, vnr);
   
    // Calculate inv_X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_calc_inv_X_energy(state, ch);
    }
   
    // Adapt H_hat
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        // There's only enough memory to store IC_X_CHANNELS worth of T data and not IC_Y_CHANNELS*IC_X_CHANNELS so the y_channels for loop cannot be run in parallel
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            ic_compute_T(state, ych, xch);

        }
        ic_filter_adapt(state);
    }

    // Apply H_hat leakage to slowly forget adaption
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        ic_apply_leakage(state, ych);
    }
}
