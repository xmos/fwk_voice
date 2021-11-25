// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "aec_config.h"
#include "aec_schedule.h"

extern "C"{
    #include "aec_defines.h"
    #include "aec_api.h"
}

extern schedule_t sch;
static unsigned X_energy_recalc_bin = 0;
static int framenum = 0;
void aec_testapp_process_frame(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2])
{
    unsafe {      
    int num_x_channels = main_state->shared_state->num_x_channels;
    int num_y_channels = main_state->shared_state->num_y_channels;

    aec_frame_init(main_state, shadow_state, y_data, x_data);

    //API: calculate average td energy (per channel)
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_update_td_ema_energy(&main_state->shared_state->y_ema_energy[ch], &main_state->shared_state->y[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
            }
        }
    }
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_x_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_update_td_ema_energy(&main_state->shared_state->x_ema_energy[ch], &main_state->shared_state->x[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
            }
        }
    }
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_fft(&main_state->shared_state->Y[ch], &main_state->shared_state->y[ch]);
            }
        }
    }
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_x_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_fft(&main_state->shared_state->X[ch], &main_state->shared_state->x[ch]);
            }
        }
    }
    //Update X_energy
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_2_tasks_and_channels[t][i].task;
            int ch = sch.par_2_tasks_and_channels[t][i].channel;
            if(ch >= num_x_channels) continue;
            int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_update_total_X_energy((aec_state_t * unsafe)main_state, ch, X_energy_recalc_bin);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_update_total_X_energy((aec_state_t * unsafe)shadow_state, ch, X_energy_recalc_bin);
                    }
                }
            }
        }
    }
    X_energy_recalc_bin += 1;
    if(X_energy_recalc_bin == (AEC_PROC_FRAME_LENGTH/2) + 1) {
        X_energy_recalc_bin = 0;
    }
    //update X_fifo with the new X and calcualate sigma_XX
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_x_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_update_X_fifo_and_calc_sigmaXX((aec_state_t * unsafe)main_state, ch);
            }
        }
    }
    //Update the 1 dimensional bfp structs that are also used to access X_fifo
    aec_update_X_fifo_1d(main_state);
    aec_update_X_fifo_1d(shadow_state);

    //calculate Error and Y_hat
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_2_tasks_and_channels[t][i].task;
            int y_ch = sch.par_2_tasks_and_channels[t][i].channel;
            if(y_ch >= num_y_channels) continue;
            int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_calc_Error_and_Y_hat((aec_state_t * unsafe)main_state, y_ch);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_calc_Error_and_Y_hat((aec_state_t * unsafe)shadow_state, y_ch);
                    }
                }
            }
        }
    }

    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_3_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_3_tasks_and_channels[t][i].task;
            int ch = sch.par_3_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_3_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_ifft(&main_state->error[ch], &main_state->Error[ch]);
                }
                else if(task == 1){
                        aec_ifft(&main_state->y_hat[ch], &main_state->Y_hat[ch]);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_ifft(&shadow_state->error[ch], &shadow_state->Error[ch]);
                    }
                }
            }
        }
    }

    //calculate coh and soh_slow 
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_calc_coherence((aec_state_t * unsafe)main_state, ch); 
            }
        }
    }

    //Window error. Calculate output
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_2_tasks_and_channels[t][i].task;
            int ch = sch.par_2_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_create_output((aec_state_t * unsafe)main_state, ch);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_create_output((aec_state_t * unsafe)shadow_state, ch);
                    }
                }
            }
        }
    }

    //calculate error_ema_energy for main state
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_1_TASKS_AND_CHANNELS_PASSES; i++) {
            int ch = sch.par_1_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_1_tasks_and_channels[t][i].is_active;
            if(is_active) {
                aec_update_td_ema_energy(&main_state->error_ema_energy[ch], &main_state->output[ch], 0, AEC_FRAME_ADVANCE, &main_state->shared_state->config_params);
            }
        }
    }
    //error -> Error FFT
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_2_tasks_and_channels[t][i].task;
            int ch = sch.par_2_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_fft(&main_state->Error[ch], &main_state->error[ch]); //error -> Error
                }
                else {
                    if(shadow_state != NULL) {
                        aec_fft(&shadow_state->Error[ch], &shadow_state->error[ch]); //error_shad -> Error_shad
                    }
                }
            }
        }
    }
    //calculate overall_Error and overall_Y
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_3_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_3_tasks_and_channels[t][i].task;
            int ch = sch.par_3_tasks_and_channels[t][i].channel;
            if(ch >= num_y_channels) continue;
            int is_active = sch.par_3_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_calc_fd_frame_energy(&main_state->overall_Error[ch], &main_state->Error[ch]);
                }
                else if(task == 1){
                        aec_calc_fd_frame_energy(&main_state->shared_state->overall_Y[ch], &main_state->shared_state->Y[ch]);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_calc_fd_frame_energy(&shadow_state->overall_Error[ch], &shadow_state->Error[ch]);
                    }
                }
            }
        }
    }
    //compare and update filters
    aec_compare_filters_and_calc_mu(main_state, shadow_state);

    //calculate inv_X_energy
    par(int t=0; t<AEC_THREAD_COUNT; t++) {
        for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
            int task = sch.par_2_tasks_and_channels[t][i].task;
            int ch = sch.par_2_tasks_and_channels[t][i].channel;
            if(ch >= num_x_channels) continue;
            int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
            if(is_active) {
                if(task == 0) {
                    aec_calc_inv_X_energy((aec_state_t * unsafe)main_state, ch, 0);
                }
                else {
                    if(shadow_state != NULL) {
                        aec_calc_inv_X_energy((aec_state_t * unsafe)shadow_state, ch, 1);
                    }
                }
            }
        }
    }

    //Adapt H_hat
    for(unsigned y_ch=0; y_ch<num_y_channels; y_ch++) {//There's only enough memory to store num_x_channels worth of T data and not num_y_channels*num_x_channels so the y_channels for loop cannot be run in parallel
        par(int t=0; t<AEC_THREAD_COUNT; t++) {
            for(int i=0; i<AEC_2_TASKS_AND_CHANNELS_PASSES; i++) {
                int task = sch.par_2_tasks_and_channels[t][i].task;
                int x_ch = sch.par_2_tasks_and_channels[t][i].channel;
                if(x_ch >= num_x_channels) continue;
                int is_active = sch.par_2_tasks_and_channels[t][i].is_active;
                if(is_active) {
                    if(task == 0) {
                        aec_compute_T((aec_state_t * unsafe)main_state, y_ch, x_ch);
                    }
                    else {
                        if(shadow_state != NULL) {
                            aec_compute_T((aec_state_t * unsafe)shadow_state, y_ch, x_ch);
                        }
                    }
                }
            }
        }
        par(int t=0; t<AEC_THREAD_COUNT; t++) {
            for(int i=0; i<AEC_2_TASKS_PASSES; i++) {
                int task = sch.par_2_tasks[t][i].task;
                int is_active = sch.par_2_tasks[t][i].is_active;
                if(is_active) {
                    if(task == 0) {
                        aec_filter_adapt((aec_state_t * unsafe)main_state, y_ch);
                    }
                    else
                    {
                        if(shadow_state != NULL) {
                            aec_filter_adapt((aec_state_t * unsafe)shadow_state, y_ch);
                        }
                    }
                }
            }
        }
    }
    framenum++; 
    }
}
