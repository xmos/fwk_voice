// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <string.h>

#include "ic_low_level.h"
#define Q1_30(f) ((int32_t)((double)(INT_MAX>>1) * (f))) //TODO use lib_xs3_math use_exponent instead when implemented

extern void ic_noise_minimisation(ic_state_t *state);

//For use when dumping variables for debug
void ic_dump_var_2d(ic_state_t *state);
void ic_dump_var_3d(ic_state_t *state);


static void ic_init_config(ic_config_params_t *config){
    config->sigma_xx_shift = IC_INIT_SIGMA_XX_SHIFT;
    config->gamma_log2 = IC_INIT_GAMMA_LOG2;
    config->ema_alpha_q30 = Q1_30(IC_INIT_EMA_ALPHA);
    config->bypass = 0;
    config->delta = double_to_float_s32(IC_INIT_DELTA);

}


static void ic_init_adaption_controller_config(ic_adaption_controller_config_t *config){
    config->leakage_alpha = double_to_float_s32(IC_INIT_LEAKAGE_ALPHA);
    config->voice_chance_alpha = double_to_float_s32(IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA);
    config->energy_alpha_slow_q30 = Q1_30(IC_INIT_ENERGY_ALPHA_SLOW);
    config->energy_alpha_fast_q30 = Q1_30(IC_INIT_ENERGY_ALPHA_FAST);

    config->out_to_in_ratio_limit = double_to_float_s32(IC_INIT_INSTABILITY_RATIO_LIMIT);
    config->instability_recovery_leakage_alpha = double_to_float_s32(IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA);

    config->enable_adaption = 1;
    config->enable_adaption_controller = 1;
    config->enable_filter_instability_recovery = IC_INIT_ENABLE_FILTER_INSTABILITY_RECOVERY;

}

static void ic_init_adaption_controller(ic_adaption_controller_state_t *adaption_controller_state){
    adaption_controller_state->smoothed_voice_chance = double_to_float_s32(IC_INIT_SMOOTHED_VOICE_CHANCE);


    adaption_controller_state->input_energy_slow = double_to_float_s32(0.0);
    adaption_controller_state->output_energy_slow = double_to_float_s32(0.0);

    adaption_controller_state->input_energy_fast = double_to_float_s32(0.0);
    adaption_controller_state->output_energy_fast = double_to_float_s32(0.0);

    ic_init_adaption_controller_config(&adaption_controller_state->adaption_controller_config);
}

void ic_init(ic_state_t *state){
    memset(state, 0, sizeof(ic_state_t));
    const exponent_t zero_exp = -1024;
    
    //H_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        for(unsigned ph=0; ph<(IC_X_CHANNELS * IC_FILTER_PHASES); ph++) {
            bfp_complex_s32_init(&state->H_hat_bfp[ch][ph], state->H_hat[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
        }
    }
    //X_fifo
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        for(unsigned ph=0; ph<IC_FILTER_PHASES; ph++) {
            bfp_complex_s32_init(&state->X_fifo_bfp[ch][ph], state->X_fifo[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
            bfp_complex_s32_init(&state->X_fifo_1d_bfp[ch * IC_FILTER_PHASES + ph], state->X_fifo[ch][ph], zero_exp, IC_FD_FRAME_LENGTH, 0);
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Error_bfp[ch], state->Error[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
        bfp_s32_init(&state->error_bfp[ch], (int32_t *)state->Error[ch], zero_exp, IC_FRAME_LENGTH, 0);
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Y_hat_bfp[ch], state->Y_hat[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    //X_energy 
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->X_energy_bfp[ch], state->X_energy[ch], zero_exp, IC_FD_FRAME_LENGTH, 0); 
    }
    state->X_energy_recalc_bin = 0;

    //sigma_XX
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->sigma_XX_bfp[ch], state->sigma_XX[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    //inv_X_energy
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->inv_X_energy_bfp[ch], state->inv_X_energy[ch], zero_exp, IC_FD_FRAME_LENGTH, 0); 
    }

    //overlap
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->overlap_bfp[ch], state->overlap[ch], zero_exp, IC_FRAME_OVERLAP, 0);
    }

    //Y, note in-place with y
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->y_bfp[ch], state->y[ch], zero_exp, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->Y_bfp[ch], (complex_s32_t*)state->y[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
        state->y_delay_idx[ch] = 0; //init delay index 
    }
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_y_bfp[ch], state->y_prev_samples[ch], zero_exp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    //X, note in-place with x
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->x_bfp[ch], state->x[ch], zero_exp, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->X_bfp[ch], (complex_s32_t*)state->x[ch], zero_exp, IC_FD_FRAME_LENGTH, 0);
    }
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_x_bfp[ch], state->x_prev_samples[ch], zero_exp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    //Reuse X memory for calculating T. Note we re-initialise T_bfp in ic_frame_init()
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->T_bfp[ch], (complex_s32_t*)&state->x_bfp[ch].data[0], 0, IC_FD_FRAME_LENGTH, 0);
    }

    //Initialise ema energy
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        state->error_ema_energy[ch].exp = zero_exp;
    }

    //Mu
    for(unsigned ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(unsigned xch=0; xch<IC_X_CHANNELS; xch++) {
            state->mu[ych][xch] = double_to_float_s32(IC_INIT_MU);
        }
    }

    //Initialise ic core config params and adaption controller
    ic_init_config(&state->config_params);
    ic_init_adaption_controller(&state->ic_adaption_controller_state);
    
    // Initialise ic_vnr_pred_state
    vnr_feature_state_init(&state->ic_vnr_pred_state.feature_state[0]);
    vnr_feature_state_init(&state->ic_vnr_pred_state.feature_state[1]);
    vnr_inference_init(&state->ic_vnr_pred_state.inference_state);
    state->ic_vnr_pred_state.pred_alpha_q30 = Q1_30(0.7);
    state->ic_vnr_pred_state.input_vnr_pred = double_to_float_s32(0.0);
    state->ic_vnr_pred_state.output_vnr_pred = double_to_float_s32(0.0);

    // chopped_y 
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->chopped_y_bfp[ch], state->chopped_y[ch], zero_exp, IC_FRAME_LENGTH, 0);
        bfp_s32_init(&state->error_copy_bfp[ch], state->error_copy[ch], zero_exp, IC_FRAME_LENGTH, 0);
    }
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

    ///Delay y channel, necessary for operation of adaptive filter
    ic_delay_y_input(state, y_data);

    ///Build a time domain frame of IC_FRAME_LENGTH from IC_FRAME_ADVANCE new samples
    ic_frame_init(state, y_data, x_data);

    ///calculate input td ema energies
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_update_td_ema_energy(&state->ic_adaption_controller_state.input_energy_slow, &state->y_bfp[ch], IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE,
                                state->ic_adaption_controller_state.adaption_controller_config.energy_alpha_slow_q30);
        ic_update_td_ema_energy(&state->ic_adaption_controller_state.input_energy_fast, &state->y_bfp[ch], IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE, 
                                state->ic_adaption_controller_state.adaption_controller_config.energy_alpha_fast_q30);
    }

    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Y_bfp[ch], &state->y_bfp[ch]);
    }

    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_fft(&state->X_bfp[ch], &state->x_bfp[ch]);
    }

    //Update X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_energy(state, ch, state->X_energy_recalc_bin);
    }

    state->X_energy_recalc_bin += 1;
    if(state->X_energy_recalc_bin == IC_FD_FRAME_LENGTH) {
        state->X_energy_recalc_bin = 0;
    }

    //update X_fifo with the new X and calcualate sigma_XX
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_fifo_and_calc_sigmaXX(state, ch);
    }

    //Update the 1 dimensional bfp structs that are also used to access X_fifo
    ic_update_X_fifo_1d(state);

    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_calc_Error_and_Y_hat(state, ch);
    }

    //IFFT Error (output)
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_ifft(&state->error_bfp[ch], &state->Error_bfp[ch]);
    }

#if !(IC_X86_BUILD)
    ic_noise_minimisation(state);
#endif

    //Note the model supports noise minimisation but we have not ported this
    //as it has not been found to aid ASR performance

    //Window error. Calculate output
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_create_output(state, output, ch);
    }

    ///calculate output td ema energies
    ic_update_td_ema_energy(&state->ic_adaption_controller_state.output_energy_slow, state->error_bfp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE,
                            state->ic_adaption_controller_state.adaption_controller_config.energy_alpha_slow_q30);
    ic_update_td_ema_energy(&state->ic_adaption_controller_state.output_energy_fast, state->error_bfp, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE, 
                            state->ic_adaption_controller_state.adaption_controller_config.energy_alpha_fast_q30);

}


void ic_adapt(
        ic_state_t *state,
        uint8_t vad,
        int32_t output[IC_FRAME_ADVANCE]){

    if(state == NULL) {
        return;
    }
   
    //Calculate leakage and mu for adaption
    ic_adaption_controller(state, vad);
   
    //calculate error_ema_energy for main state
    const exponent_t q0_31_exp = -31;
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_t temp;
        bfp_s32_init(&temp, output, q0_31_exp, IC_FRAME_ADVANCE, 1);

        ic_update_td_ema_energy(&state->error_ema_energy[ch], &temp, 0, IC_FRAME_ADVANCE, state->config_params.ema_alpha_q30);
    }
   
    //error -> Error FFT
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Error_bfp[ch], &state->error_bfp[ch]);
    }
   
    //calculate inv_X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_calc_inv_X_energy(state, ch);
    }
   
    //Adapt H_hat
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        //There's only enough memory to store IC_X_CHANNELS worth of T data and not IC_Y_CHANNELS*IC_X_CHANNELS so the y_channels for loop cannot be run in parallel
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            ic_compute_T(state, ych, xch);

        }
        ic_filter_adapt(state);
    }

    //Apply H_hat leakage to slowly forget adaption
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        ic_apply_leakage(state, ych);
    }
}

void ic_calc_vnr_pred(ic_state_t *state)
{
    ic_priv_calc_vnr_pred(&state->ic_vnr_pred_state, &state->Y_bfp[0], &state->Error_bfp[0]);
}
