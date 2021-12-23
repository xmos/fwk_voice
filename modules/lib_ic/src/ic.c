// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>

#include "ic_low_level.h"


void ic_init(ic_state_t *state){

    memset(state, 0, sizeof(ic_state_t));
    
    //H_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        for(unsigned ph=0; ph<(IC_X_CHANNELS * IC_FILTER_PHASES); ph++) {
            bfp_complex_s32_init(&state->H_hat_bfp[ch][ph], state->H_hat[ch][ph], -1024, IC_FD_FRAME_LENGTH, 0);
            printf("H_hat ph(%u) len: %u\n", ph, state->H_hat_bfp[ch][ph].length);
        }
    }
    //X_fifo
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        for(unsigned ph=0; ph<IC_FILTER_PHASES; ph++) {
            bfp_complex_s32_init(&state->X_fifo_bfp[ch][ph], state->X_fifo[ch][ph], -1024, IC_FD_FRAME_LENGTH, 0);
            bfp_complex_s32_init(&state->X_fifo_1d_bfp[ch * IC_FILTER_PHASES + ph], state->X_fifo[ch][ph], -1024, IC_FD_FRAME_LENGTH, 0);
        }
    }
    //initialise Error
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Error_bfp[ch], state->Error[ch], -1024, IC_FD_FRAME_LENGTH, 0);
        bfp_s32_init(&state->error_bfp[ch], (int32_t *)state->Error[ch], -1024, IC_FRAME_LENGTH, 0);
    }
    //Initiaise Y_hat
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_complex_s32_init(&state->Y_hat_bfp[ch], state->Y_hat[ch], -1024, IC_FD_FRAME_LENGTH, 0);
        bfp_s32_init(&state->y_hat_bfp[ch], (int32_t *)state->Y_hat[ch], -1024, IC_FRAME_LENGTH, 0);
    }
    //X_energy 
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->X_energy_bfp[ch], state->X_energy[ch], -1024, IC_FD_FRAME_LENGTH, 0); 
    }
    state->X_energy_recalc_bin = 0;

    //sigma_XX
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->sigma_XX_bfp[ch], state->sigma_XX[ch], -1024, IC_FD_FRAME_LENGTH, 0);
    }
    //inv_X_energy
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->inv_X_energy_bfp[ch], state->inv_X_energy[ch], -1024, IC_FD_FRAME_LENGTH, 0); 
    }

    //overlap
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->overlap_bfp[ch], state->overlap[ch], -1024, 32, 0);
    }

    //Y, note in-place with y
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->y_bfp[ch], state->y[ch], -1024, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->Y_bfp[ch], (complex_s32_t*)state->y[ch], -1024, IC_FD_FRAME_LENGTH, 0);
    }
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_y_bfp[ch], state->y_prev_samples[ch], -1024, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    //X, note in-place with x
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->x_bfp[ch], state->x[ch], -1024, IC_FRAME_LENGTH, 0);
        bfp_complex_s32_init(&state->X_bfp[ch], (complex_s32_t*)state->x[ch], -1024, IC_FD_FRAME_LENGTH, 0);
    }
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        bfp_s32_init(&state->prev_x_bfp[ch], state->x_prev_samples[ch], -1024, IC_FRAME_LENGTH - IC_FRAME_ADVANCE, 0);
    }

    //Initialise ema energy
    for(unsigned ch=0; ch<IC_Y_CHANNELS; ch++) {
        state->y_ema_energy[ch].exp = -1024;
        state->error_ema_energy[ch].exp = -1024;
    }
    for(unsigned ch=0; ch<IC_X_CHANNELS; ch++) {
        state->x_ema_energy[ch].exp = -1024;
    }
    //fractional regularisation scalefactor
    state->delta = IC_INIT_DELTA;

    state->config_params.core_conf.sigma_xx_shift = IC_INIT_SIGMA_XX_SHIFT;
    state->config_params.core_conf.gamma_log2 = IC_INIT_GAMMA_LOG2;
    state->config_params.core_conf.ema_alpha_q30 = IC_INIT_EMA_ALPHA_Q30;
    state->config_params.core_conf.bypass = 0;
    state->config_params.core_conf.coeff_index = 0;

    state->ic_adaption_controller_state.leakage_alpha = IC_INIT_LEAKAGE_ALPHA;
    state->ic_adaption_controller_state.adaption_mode = IC_ADAPTION_FORCE_OFF;

    state->ic_adaption_controller_state.vad_counter = 0;
    state->ic_adaption_controller_state.smoothed_voice_chance = IC_INIT_SMOOTHED_VOICE_CHANCE;
    state->ic_adaption_controller_state.voice_chance_alpha = IC_INIT_SMOOTHED_VOICE_CHANCE_ALPHA;

    state->ic_adaption_controller_state.energy_alpha = IC_INIT_ENERGY_ALPHA;
    state->ic_adaption_controller_state.input_energy = double_to_float_s32(0.0);
    state->ic_adaption_controller_state.output_energy = double_to_float_s32(0.0);

    state->ic_adaption_controller_state.energy_alpha0 = IC_INIT_ENERGY_ALPHA0;
    state->ic_adaption_controller_state.input_energy0 = double_to_float_s32(0.0);
    state->ic_adaption_controller_state.output_energy0 = double_to_float_s32(0.0);

    state->ic_adaption_controller_state.out_to_in_ratio_limit = double_to_float_s32(1.0);
    state->ic_adaption_controller_state.enable_filter_instability_recovery = IC_INIT_ENABLE_FILTER_INSTABILITY_RECOVERY;
    state->ic_adaption_controller_state.instability_recovery_leakage_alpha = IC_INIT_INSTABILITY_RECOVERY_LEAKAGE_ALPHA;

    for(unsigned ych=0; ych<IC_Y_CHANNELS; ych++) {
        for(unsigned xch=0; xch<IC_X_CHANNELS; xch++) {
            mu[ych][xch] = IC_INIT_MU;
        }
    }


    //Initialise ic config params
    // ic_priv_init_config_params(&state->config_params);
}

void ic_process_frame(
        ic_state_t *state,
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE],
        uint8_t vad,
        int32_t output[IC_FRAME_ADVANCE])
{

    printf("state.config_params.core_conf.bypass: %d\n", state->config_params.core_conf.bypass);
    printf("offset: %u\n", (unsigned)&state->config_params.core_conf.bypass - (unsigned)state);


    ic_frame_init(state, y_data, x_data);
    printf("ic_frame_init\n");

    ///calculate input td ema energy
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_update_td_ema_energy(&state->y_ema_energy[ch], &state->y_bfp[ch], IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE, &state->config_params);
    }
    printf("ic_update_td_ema_energy\n");

    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_td_ema_energy(&state->x_ema_energy[ch], &state->x_bfp[ch], IC_FRAME_LENGTH - IC_FRAME_ADVANCE, IC_FRAME_ADVANCE, &state->config_params);
    }
    printf("ic_update_td_ema_energy\n");


    printf("y_ema_energy: %f\n", ldexp(state->y_ema_energy[0].mant, state->y_ema_energy[0].exp));
    printf("x_ema_energy: %f\n", ldexp(state->x_ema_energy[0].mant, state->x_ema_energy[0].exp));


    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Y_bfp[ch], &state->y_bfp[ch]);
    }
    printf("ic_fft\n");


    for(int i=0; i<257; i++) {
        // printf("Y_data: %.12f + %.12fj\n", ldexp( state->Y_bfp[0].data[i].re, state->Y_bfp[0].exp), ldexp( state->Y_bfp[0].data[i].im, state->Y_bfp[0].exp));

        // printf("y: %.12f\n", ldexp( state->y_bfp[0].data[i], state->y_bfp[0].exp));
        }
    printf("\n\n");

    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_fft(&state->X_bfp[ch], &state->x_bfp[ch]);
    }
    printf("ic_fft\n");

    //Update X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_energy(state, ch, state->X_energy_recalc_bin);
    }

    state->X_energy_recalc_bin += 1;
    if(state->X_energy_recalc_bin == IC_FD_FRAME_LENGTH) {
        state->X_energy_recalc_bin = 0;
    }
    printf("ic_update_X_energy\n");

    //update X_fifo with the new X and calcualate sigma_XX
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_update_X_fifo_and_calc_sigmaXX(state, ch);
    }
    printf("ic_update_X_fifo_and_calc_sigmaXX\n");

    //Update the 1 dimensional bfp structs that are also used to access X_fifo
    ic_update_X_fifo_1d(state);
    printf("ic_update_X_fifo_1d\n");


    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_calc_Error_and_Y_hat(state, ch);
    }
    printf("ic_calc_Error_and_Y_hat\n");

        for(int i=0; i<257; i++) {
        printf("Error: %.12f + %.12fj\n", ldexp( state->Error_bfp[0].data[i].re, state->Error_bfp[0].exp), ldexp( state->Error_bfp[0].data[i].im, state->Error_bfp[0].exp));
        }

    //IFFT Error and Y_hat
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_ifft(&state->error_bfp[ch], &state->Error_bfp[ch]);
        ic_ifft(&state->y_hat_bfp[ch], &state->Y_hat_bfp[ch]);
    }
    printf("ic_ifft\n");

    //Window error. Calculate output
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_create_output(state, output, ch);
    }
    printf("ic_create_output\n");

    //Process vad input
    ic_update_vad_history(state, output, &vad);
    printf("ic_update_vad_history\n");

    //Calculate leakage and mu for adaption
    ic_adaption_controller(state, vad);
    printf("ic_adaption_controller\n");

    //calculate error_ema_energy for main state
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        bfp_s32_t temp;
        bfp_s32_init(&temp, output, -31, IC_FRAME_ADVANCE, 1);

        ic_update_td_ema_energy(&state->error_ema_energy[ch], &temp, 0, IC_FRAME_ADVANCE, &state->config_params);
    }
    printf("ic_update_td_ema_energy\n");
    printf("error_ema_energy: %f\n", ldexp(state->error_ema_energy[0].mant, state->error_ema_energy[0].exp));


    //error -> Error FFT
    for(int ch=0; ch<IC_Y_CHANNELS; ch++) {
        ic_fft(&state->Error_bfp[ch], &state->error_bfp[ch]);
    }
    printf("ic_fft\n");

    //calculate inv_X_energy
    for(int ch=0; ch<IC_X_CHANNELS; ch++) {
        ic_calc_inv_X_energy(state, ch);
    }
    printf("ic_calc_inv_X_energy\n");


    //Adapt H_hat
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        //There's only enough memory to store IC_X_CHANNELS worth of T data and not IC_Y_CHANNELS*IC_X_CHANNELS so the y_channels for loop cannot be run in parallel
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            ic_compute_T(state, ych, xch);

            // for(int i=0; i<state->T_bfp[xch].length; i++) {
            for(int i=0; i<10; i++) {
                // printf("T:     %.12f + %.12fj\n", ldexp( state->T_bfp[xch].data[i].re, state->T_bfp[xch].exp), ldexp( state->T_bfp[xch].data[i].im, state->T_bfp[xch].exp));
                // printf("Inv_x: %.12f\n", ldexp( state->inv_X_energy_bfp[xch].data[i], state->inv_X_energy_bfp[xch].exp));
                printf("error: %.12f\n", ldexp( state->error_bfp[ych].data[i], state->error_bfp[ych].exp));
                // printf("mu: %.12f\n", ldexp( state->mu[ych][xch].mant, state->mu[ych][xch].exp));
                }
            printf("\n\n");


        }
        printf("ic_compute_T\n");

        ic_filter_adapt(state);
        printf("ic_filter_adapt\n");

    }

    //Apply H_hat leakage
    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {
        ic_apply_leakage(state, ych);
        printf("ic_apply_leakage\n");
    }

}
