// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "aec_config.h"
#include "aec_schedule.h"

#include "aec_defines.h"
#include "aec_api.h"

enum e_td_ema {Y_EMA, X_EMA, ERROR_EMA};
enum e_fft {Y_FFT, X_FFT, ERROR_FFT};

#if AEC_THREAD_COUNT > 1
#include <xcore/parallel.h>
DECLARE_JOB(update_td_ema_energy_task, (par_tasks_and_channels_t*, aec_state_t *, int, int, enum e_td_ema));
DECLARE_JOB(fft_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, enum e_fft));
DECLARE_JOB(update_X_energy_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, int));
DECLARE_JOB(update_X_fifo_task, (par_tasks_and_channels_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_Error_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(ifft_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_coh_task, (par_tasks_and_channels_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_output_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_fd_energy_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_inv_X_energy_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_T_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, int));
DECLARE_JOB(filter_adapt_task, (par_tasks_t*, aec_state_t*, aec_state_t*, int, int));
#endif

void update_td_ema_energy_task(par_tasks_and_channels_t* s, aec_state_t *state, int passes, int channels, enum e_td_ema type) {
    for(int i=0; i<passes; i++) {
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(type == Y_EMA) {
                aec_update_td_ema_energy(&state->shared_state->y_ema_energy[ch], &state->shared_state->y[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
            }
            else if(type == X_EMA) {
                aec_update_td_ema_energy(&state->shared_state->x_ema_energy[ch], &state->shared_state->x[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
            }
            else if(type == ERROR_EMA) {
                aec_update_td_ema_energy(&state->error_ema_energy[ch], &state->output[ch], 0, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
            }
            else{assert(0);}
        }
    }
}

void fft_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels, enum e_fft type) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(type == Y_FFT) {
                aec_fft(
                        &main_state->shared_state->Y[ch],
                        &main_state->shared_state->y[ch]);
            }
            else if(type == X_FFT) {
                aec_fft(
                        &main_state->shared_state->X[ch],
                        &main_state->shared_state->x[ch]);
            }
            else if((type==ERROR_FFT) && (task==0)) {
                aec_fft(
                        &main_state->Error[ch],
                        &main_state->error[ch]
                        ); //error -> Error
            }
            else if((type==ERROR_FFT) && (task==1) && (shadow_state != NULL)){
                aec_fft(
                        &shadow_state->Error[ch],
                        &shadow_state->error[ch]
                        ); //error_shad -> Error_shad
            }
            else{assert(0);}
        }
    }
}

void update_X_energy_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels, int recalc_bin) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_update_total_X_energy(main_state, ch, recalc_bin);
            }
            else {
                if(shadow_state != NULL) {
                    aec_update_total_X_energy(shadow_state, ch, recalc_bin);
                }
            }
        }
    }
}

void update_X_fifo_task(par_tasks_and_channels_t *s, aec_state_t *state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            aec_update_X_fifo_and_calc_sigmaXX(state, ch);
        }
    }
}

void calc_Error_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_calc_Error_and_Y_hat(main_state, ch);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_Error_and_Y_hat(shadow_state, ch);
                }
            }
        }
    }
}

void ifft_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels)
{
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
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
void calc_coh_task(par_tasks_and_channels_t *s, aec_state_t *state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            aec_calc_coherence(state, ch);
        }
    }
}

void calc_output_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_create_output(main_state, ch);
            }
            else {
                if(shadow_state != NULL) {
                    aec_create_output(shadow_state, ch);
                }
            }
        }
    }
}

void calc_fd_energy_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels)
{
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
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

void calc_inv_X_energy_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_calc_inv_X_energy(main_state, ch, 0);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_inv_X_energy(shadow_state, ch, 1);
                }
            }
        }
    }
}

void calc_T_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels, int ych) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int xch = s[i].channel;
        if(xch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_compute_T(main_state, ych, xch);
            }
            else {
                if(shadow_state != NULL) {
                    aec_compute_T(shadow_state, ych, xch);
                }
            }
        }
    }
}

void filter_adapt_task(par_tasks_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int ych) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_filter_adapt(main_state, ych);
            }
            else
            {
                if(shadow_state != NULL) {
                    aec_filter_adapt(shadow_state, ych);
                }
            }
        }
    }
}

#if AEC_THREAD_COUNT < 1 || AEC_THREAD_COUNT > 2
#error "C app only build for AEC_THREAD_COUNT range [1, 2]"
#endif

extern schedule_t sch;
static unsigned X_energy_recalc_bin = 0;
static int framenum = 0;
void aec_testapp_process_frame(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*y_data)[AEC_PROC_FRAME_LENGTH+2],
        int32_t (*x_data)[AEC_PROC_FRAME_LENGTH+2])
{
    int num_x_channels = main_state->shared_state->num_x_channels;
    int num_y_channels = main_state->shared_state->num_y_channels;

    aec_frame_init(main_state, shadow_state, y_data, x_data);

    ///calculate input td ema energy
#if AEC_THREAD_COUNT == 1
    update_td_ema_energy_task(sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_EMA);
#else
    PAR_JOBS(
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_EMA)),
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_EMA))
        );
#endif

#if AEC_THREAD_COUNT == 1
    update_td_ema_energy_task(sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_EMA);
#else
    PAR_JOBS(
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_EMA)),
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_EMA))
        );
#endif

#if AEC_THREAD_COUNT == 1
    fft_task(sch.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_FFT);
#else
    ///Input FFT
    PAR_JOBS(
        PJOB(fft_task, (sch.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_FFT)),
        PJOB(fft_task, (sch.par_1_tasks_and_channels[1], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_FFT))
        );
#endif

#if AEC_THREAD_COUNT == 1
    fft_task(sch.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_FFT);
#else
    PAR_JOBS(
        PJOB(fft_task, (sch.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_FFT)),
        PJOB(fft_task, (sch.par_1_tasks_and_channels[1], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_FFT))
        );
#endif

    //Update X_energy
#if AEC_THREAD_COUNT == 1
    update_X_energy_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_energy_recalc_bin);
#else
    PAR_JOBS(
        PJOB(update_X_energy_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_energy_recalc_bin)),
        PJOB(update_X_energy_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_energy_recalc_bin))
        );
#endif
    X_energy_recalc_bin += 1;
    if(X_energy_recalc_bin == (AEC_PROC_FRAME_LENGTH/2) + 1) {
        X_energy_recalc_bin = 0;
    }

    //update X_fifo with the new X and calcualate sigma_XX
#if AEC_THREAD_COUNT == 1
    update_X_fifo_task(sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels);
#else
    PAR_JOBS(
        PJOB(update_X_fifo_task, (sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels)),
        PJOB(update_X_fifo_task, (sch.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels))
        );
#endif

    //Update the 1 dimensional bfp structs that are also used to access X_fifo
    aec_update_X_fifo_1d(main_state);
    aec_update_X_fifo_1d(shadow_state);

    //calculate Error and Y_hat
#if AEC_THREAD_COUNT == 1
    calc_Error_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels);
#else
    PAR_JOBS(
        PJOB(calc_Error_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_Error_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
#endif
    
    //IFFT Error and Y_hat
#if AEC_THREAD_COUNT == 1
    ifft_task(sch.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels);
#else
    PAR_JOBS(
        PJOB(ifft_task, (sch.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(ifft_task, (sch.par_3_tasks_and_channels[1], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
#endif

    //calculate coh and coh_slow 
#if AEC_THREAD_COUNT == 1
    calc_coh_task(sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels);
#else
    PAR_JOBS(
        PJOB(calc_coh_task, (sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_coh_task, (sch.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
#endif

    //Window error. Calculate output
#if AEC_THREAD_COUNT == 1
    calc_output_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels);
#else
    PAR_JOBS(
        PJOB(calc_output_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_output_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
#endif

    //calculate error_ema_energy for main state
#if AEC_THREAD_COUNT == 1
    update_td_ema_energy_task(sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_EMA);
#else
    PAR_JOBS(
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_EMA)),
        PJOB(update_td_ema_energy_task, (sch.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_EMA))
        );
#endif

    //error -> Error FFT
#if AEC_THREAD_COUNT == 1
    fft_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_FFT);
#else
    PAR_JOBS(
        PJOB(fft_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_FFT)),
        PJOB(fft_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_FFT))
            );
#endif

    //calculate overall_Error and overall_Y
#if AEC_THREAD_COUNT == 1
    calc_fd_energy_task(sch.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels);
#else
    PAR_JOBS(
        PJOB(calc_fd_energy_task, (sch.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_fd_energy_task, (sch.par_3_tasks_and_channels[1], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
#endif

    //compare and update filters
    aec_compare_filters_and_calc_mu(
            main_state,
            shadow_state);

    //calculate inv_X_energy
#if AEC_THREAD_COUNT == 1
    calc_inv_X_energy_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels);
#else
    PAR_JOBS(
        PJOB(calc_inv_X_energy_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels)),
        PJOB(calc_inv_X_energy_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels))
        );
#endif

    //Adapt H_hat
    for(int ych=0; ych<num_y_channels; ych++) {
        //There's only enough memory to store num_x_channels worth of T data and not num_y_channels*num_x_channels so the y_channels for loop cannot be run in parallel
#if AEC_THREAD_COUNT == 1
        calc_T_task(sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, ych);
#else
        PAR_JOBS(
            PJOB(calc_T_task, (sch.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, ych)),
            PJOB(calc_T_task, (sch.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, ych))
            );
#endif

#if AEC_THREAD_COUNT == 1
        filter_adapt_task(sch.par_2_tasks[0], main_state, shadow_state, AEC_2_TASKS_PASSES, ych);
#else
        PAR_JOBS(
            PJOB(filter_adapt_task, (sch.par_2_tasks[0], main_state, shadow_state, AEC_2_TASKS_PASSES, ych)),
            PJOB(filter_adapt_task, (sch.par_2_tasks[1], main_state, shadow_state, AEC_2_TASKS_PASSES, ych))
            );
#endif
    }
    framenum++; 
}
