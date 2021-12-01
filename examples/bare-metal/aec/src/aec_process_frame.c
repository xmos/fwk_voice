// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "aec_config.h"
#include "aec_defines.h"
#include "aec_api.h"


static unsigned X_energy_recalc_bin = 0;
static int framenum = 0;
void aec_process_frame(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2])
{
    int num_x_channels = main_state->shared_state->num_x_channels;
    int num_y_channels = main_state->shared_state->num_y_channels;

    aec_frame_init(main_state, shadow_state, y_data, x_data);
    
    //Calculate time domain input exponential moving average energy
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_update_td_ema_energy(&main_state->shared_state->y_ema_energy[ch], &main_state->shared_state->y[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
    }
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_update_td_ema_energy(&main_state->shared_state->x_ema_energy[ch], &main_state->shared_state->x[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
    }

    //Calculate input spectrum
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_fft(
                &main_state->shared_state->Y[ch],
                &main_state->shared_state->y[ch]);
    }
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_fft(
                &main_state->shared_state->X[ch],
                &main_state->shared_state->x[ch]);
    }

    //Calculate X-FIFO energy for main and shadow filter
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_update_total_X_energy(main_state, ch, X_energy_recalc_bin);
        aec_update_total_X_energy(shadow_state, ch, X_energy_recalc_bin);
    }
    X_energy_recalc_bin += 1;
    if(X_energy_recalc_bin == (AEC_PROC_FRAME_LENGTH/2) + 1) {
        X_energy_recalc_bin = 0;
    }

    //Update X-FIFO with the current frame reference input
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_update_X_fifo_and_calc_sigmaXX(main_state, ch);
    }

    //Update the 1 dimensional bfp structs that are also used to access X_fifo
    aec_update_X_fifo_1d(main_state);
    aec_update_X_fifo_1d(shadow_state);

    //Calculate error and estimated mic spectrum
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_calc_Error_and_Y_hat(main_state, ch);
        aec_calc_Error_and_Y_hat(shadow_state, ch);
    }
    
    //Calculate time domain error and estimated mic
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_ifft(&main_state->error[ch], &main_state->Error[ch]);
        aec_ifft(&main_state->y_hat[ch], &main_state->Y_hat[ch]);
        aec_ifft(&shadow_state->error[ch], &shadow_state->Error[ch]);
    }

    //Calculate mic and estimated mic coherence (coh) and slow moving coherence (coh_slow)
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_calc_coherence(main_state, ch);
    }

    //Calculate AEC output
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_create_output(main_state, ch);
        aec_create_output(shadow_state, ch);
    }

    //calculate error EMA energy for main_state
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_update_td_ema_energy(&main_state->error_ema_energy[ch], &main_state->output[ch], 0, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
    }

    //Compute error specturm for main and shadow filter
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_fft(
                &main_state->Error[ch],
                &main_state->error[ch]
               );
        
        aec_fft(
                &shadow_state->Error[ch],
                &shadow_state->error[ch]
               );
    }

    //Calculate mic input spectrum and error spectrum energy
    for(int ch=0; ch<num_y_channels; ch++) {
        aec_calc_fd_frame_energy(&main_state->overall_Error[ch], &main_state->Error[ch]);
        aec_calc_fd_frame_energy(&main_state->shared_state->overall_Y[ch], &main_state->shared_state->Y[ch]);
        aec_calc_fd_frame_energy(&shadow_state->overall_Error[ch], &shadow_state->Error[ch]);
    }

    //Compare and update filters. Calculate adaption step_size mu
    aec_compare_filters_and_calc_mu(
            main_state,
            shadow_state);

    //calculate normalised reference input spectrum
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_calc_inv_X_energy(main_state, ch, 0);
        aec_calc_inv_X_energy(shadow_state, ch, 1);
    }

    //Adapt H_hat
    for(int ych=0; ych<num_y_channels; ych++) {
        //There's only enough memory to store num_x_channels worth of T data and not num_y_channels*num_x_channels so the y_channels for loop cannot be run in parallel
        for(int xch=0; xch<num_x_channels; xch++) {
            aec_compute_T(main_state, ych, xch);
            aec_compute_T(shadow_state, ych, xch);
        }
        aec_filter_adapt(main_state, ych);
        aec_filter_adapt(shadow_state, ych);
    }
    framenum++; 
}
