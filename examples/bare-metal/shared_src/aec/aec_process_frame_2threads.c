// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "aec_task_distribution.h"

#include "aec_defines.h"
#include "aec_api.h"

/* This is a bare-metal example of processing one frame of data through the AEC pipeline stage. This example
 * demonstrates distributing AEC functions across 2 cores in parallel using lib_xcore PAR functionality.
 * Task distribution across cores is done using the scheme defined in aec_task_distribution.h and
 * aec_task_distribution.c
 */
enum e_td_ema {Y_EMA, X_EMA, ERROR_EMA};
enum e_fft {Y_FFT, X_FFT, ERROR_FFT};

#include <xcore/parallel.h>
DECLARE_JOB(calc_time_domain_ema_energy_task, (par_tasks_and_channels_t*, aec_state_t *, int32_t*, int, int, enum e_td_ema));
DECLARE_JOB(fft_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, enum e_fft));
DECLARE_JOB(update_X_energy_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, int));
DECLARE_JOB(update_X_fifo_task, (par_tasks_and_channels_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_Error_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(ifft_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_coh_task, (par_tasks_and_channels_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_output_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int32_t*, int32_t*, int, int));
DECLARE_JOB(calc_freq_domain_energy_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_normalisation_spectrum_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int));
DECLARE_JOB(calc_T_task, (par_tasks_and_channels_t*, aec_state_t*, aec_state_t*, int, int, int));
DECLARE_JOB(filter_adapt_task, (par_tasks_t*, aec_state_t*, aec_state_t*, int, int));

extern task_distribution_t tdist;
static unsigned X_energy_recalc_bin = 0;
static int framenum = 0;
void aec_process_frame_2threads(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE],    
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE])
{
    // Read number of mic and reference channels. These are specified as part of the configuration when aec_init() is called.
    int num_y_channels = main_state->shared_state->num_y_channels; //Number of mic channels
    int num_x_channels = main_state->shared_state->num_x_channels; //Number of reference channels

    // Set up the input BFP structures main_state->shared_state->y and main_state->shared_state->x to point to the new frame.
    // Initialise some other BFP structures that need to be initialised at the beginning of each frame
    aec_frame_init(main_state, shadow_state, y_data, x_data);

    // Calculate Exponential moving average (EMA) energy of the mic and reference input.
    PAR_JOBS(
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[0], main_state, NULL, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_EMA)),
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[1], main_state, NULL, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_EMA))
        );

    PAR_JOBS(
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[0], main_state, NULL, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_EMA)),
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[1], main_state, NULL, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_EMA))
        );

    // Calculate mic input spectrum for all num_y_channels of mic input
    /* The spectrum calculation is done in place. Taking mic input as example, after the aec_forward_fft() call
     * main_state->shared_state->Y[ch].data and main_state->shared_state->y[ch].data point to the same memory address.
     * The spectral representation of the input is used after this function. Time domain input
     * BFP structure main_state->shared_state->y should not be used.
     * main_state->shared_state->Y[ch].data points to AEC_PROC_FRAME_LENGTH/2 + 1 complex 32bit spectrum samples,
     * which represent the spectrum samples from DC to Nyquist frequency.
     * Same is true for reference spectrum samples pointed to by  main_state->shared_state->X[ch].data
     * as well.
     */
    PAR_JOBS(
        PJOB(fft_task, (tdist.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_FFT)),
        PJOB(fft_task, (tdist.par_1_tasks_and_channels[1], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, Y_FFT))
        );

    // Calculate reference input spectrum for all num_x_channels of reference input
    PAR_JOBS(
        PJOB(fft_task, (tdist.par_1_tasks_and_channels[0], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_FFT)),
        PJOB(fft_task, (tdist.par_1_tasks_and_channels[1], main_state, shadow_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_FFT))
        );

    // Calculate sum of X energy over X FIFO phases for all num_x_channels reference channels for main and shadow filter.   
    /* AEC data structures store a single copy of the X FIFO that is shared between the main and shadow filter.
     * Since main filter phases main_state->num_phases are more than the shadow filter phases shadow_state->num_phases,
     * X FIFO holds main_state->num_phases most recent frames of reference input spectrum, where the frames are ordered
     * from most recent to least recent. For shadow filter operation, out of this shared X FIFO, the first shadow_state->num_phases
     * frames are considered.
     *
     * For main filter, X energy is stored in BFP struct main_state->X_energy[ch]. For shadow filter, X energy is stored
     * in BFP structure shadow_state->X_energy[ch]. These BFP structures point to AEC_PROC_FRAME_LENGTH/2 + 1, real
     * 32bit values where the value at index n is the nth X sample's energy summed over main_state->num_phases number
     * of frames in the X FIFO.
     */
    PAR_JOBS(
        PJOB(update_X_energy_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_energy_recalc_bin)),
        PJOB(update_X_energy_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, X_energy_recalc_bin))
        );

    // Increment X_energy_recalc_bin to the next sample index.
    /* Passing X_energy_recalc_bin to aec_calc_X_fifo_energy() ensures that energy of sample at index X_energy_recalc_bin
     * is recalculated without the speed optimisations so that quantisation error can be kept in check
     */
    X_energy_recalc_bin += 1;
    if(X_energy_recalc_bin == (AEC_PROC_FRAME_LENGTH/2) + 1) {
        X_energy_recalc_bin = 0;
    }

    // Update X-FIFO and calculate sigma_XX.
    /* Add the current X frame to the X FIFO and remove the oldest X frame from the X FIFO.
     * Also, calculate state->shared_state->sigma_XX. sigma_XX is the EMA of current X frame energy.
     * It is later used to time smooth the X_energy while calculating the normalisation spectrum
     */
    PAR_JOBS(
        PJOB(update_X_fifo_task, (tdist.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels)),
        PJOB(update_X_fifo_task, (tdist.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_x_channels))
        );

    // Copy state->shared_state->X_fifo BFP struct to main_state->X_fifo_1d and shadow_state->X_fifo_1d BFP structs
    /* The updated state->shared_state->X_FIFO BFP structures are copied to an alternate set of BFP structs present in the 
     * main and shadow filter state structure, that are used to efficiently access the X FIFO in the Error computation and filter
     * update steps.
     */
    aec_update_X_fifo_1d(main_state);
    aec_update_X_fifo_1d(shadow_state);

    // Calculate error spectrum and estimated mic spectrum for main and shadow adaptive filters
    /* For main filter, main_state->Error[ch] and main_state->Y_hat[ch] are updated.
     * For shadow filter, shadow_state->Error[ch] and shadow_state->Y_hat[ch] are updated. 
     */
    PAR_JOBS(
        PJOB(calc_Error_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_Error_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );
    
    // Calculate time domain error and time domain estimated mic input from their spectrums calculated in the previous step.
    /* The time domain estimated mic_input (y_hat) is used to calculate the average coherence between y and y_hat in aec_calc_coherence.
     * Only the estimated mic input calculated using the main filter is needed for coherence calculation, so the y_hat calculation is
     * done only for main filter.
     */
    PAR_JOBS(
        PJOB(ifft_task, (tdist.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(ifft_task, (tdist.par_3_tasks_and_channels[1], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );

    // Calculate average coherence and average slow moving coherence between mic and estimated mic time domain signals
    // main_state->shared_state->coh_mu_state[ch].coh and main_state->shared_state->coh_mu_state[ch].coh_slow are updated
    PAR_JOBS(
        PJOB(calc_coh_task, (tdist.par_1_tasks_and_channels[0], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_coh_task, (tdist.par_1_tasks_and_channels[1], main_state, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );

    // Calculate AEC filter time domain output. This is the output sent to downstream pipeline stages
    /* Application can choose to not generate AEC shadow filter output by passing NULL as output_shadow argument.
     * Note that aec_calc_output() will still need to be called since this function also windows the error signal
     * which is needed for subsequent processing of the shadow filter even when output is not generated.
     */
    PAR_JOBS(
        PJOB(calc_output_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, (int32_t*)output_main, (int32_t*)output_shadow, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_output_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, (int32_t*)output_main, (int32_t*)output_shadow, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );

    // Calculate exponential moving average of main_filter time domain error.
    /* The EMA error energy is used in ERLE calculations which are done only for the main filter,
     * so not calling this function to calculate shadow filter error EMA energy.
     */
    PAR_JOBS(
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[0], main_state, (int32_t*)output_main, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_EMA)),
        PJOB(calc_time_domain_ema_energy_task, (tdist.par_1_tasks_and_channels[1], main_state, (int32_t*)output_main, AEC_1_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_EMA))
        );

    // Convert shadow and main filters error back to frequency domain since subsequent AEC functions will use the error spectrum.
    /* The error spectrum is later used to compute T values which are then used while updating the adaptive filter.
     * main_state->Error[ch] and shadow_state->Error[ch] are updated.
     */
    PAR_JOBS(
        PJOB(fft_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_FFT)),
        PJOB(fft_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_y_channels, ERROR_FFT))
            );

    // Calculate energies of mic input and error spectrum of main and shadow filters.
    /* These energy values are later used in aec_compare_filters_and_calc_mu() to estimate how well the filters are performing.
     * main_state->overall_Error[ch], shadow_state->overall_Error[ch] and main_state->shared_state->overall_Y[ch] are
     * updated.
     */
    PAR_JOBS(
        PJOB(calc_freq_domain_energy_task, (tdist.par_3_tasks_and_channels[0], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels)),
        PJOB(calc_freq_domain_energy_task, (tdist.par_3_tasks_and_channels[1], main_state, shadow_state, AEC_3_TASKS_AND_CHANNELS_PASSES, num_y_channels))
        );

    // Compare and update filters. Calculate adaption step_size mu
    /* At this point we're ready to check how well the filters are performing and update them if needed.
     * 
     * main_state->shared_state->shadow_filter_params are updated to indicate the current state of filter comparison algorithm.
     * main_state->H_hat, main_state->Error, shadow_state->H_hat, shadow_state->Error are optionally updated depending on the update needed.
     *
     * After the filter comparison and update step, the adaption step size mu is calculated for main and shadow filter.
     * main_state->mu and shadow_state->mu are updated.
     */
    aec_compare_filters_and_calc_mu(
            main_state,
            shadow_state);

    // Calculate smoothed reference FIFO energy that is later used to scale the X FIFO in the filter update step.
    /* This calculation is done differently for main and shadow filters, so a flag indicating filter type is specified
     * as one of the input arguments.
     * main_state->inv_X_energy[ch] and shadow_state->inv_X_energy[ch] is updated.
     */
    PAR_JOBS(
        PJOB(calc_normalisation_spectrum_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels)),
        PJOB(calc_normalisation_spectrum_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels))
        );

    //Adapt H_hat
    for(int ych=0; ych<num_y_channels; ych++) {
        // Compute T values.
        // T is a function of state->mu, state->Error and state->inv_X_energy.
        // main_state->T[ch] and shadow_state->T[ch] are updated.
        PAR_JOBS(
            PJOB(calc_T_task, (tdist.par_2_tasks_and_channels[0], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, ych)),
            PJOB(calc_T_task, (tdist.par_2_tasks_and_channels[1], main_state, shadow_state, AEC_2_TASKS_AND_CHANNELS_PASSES, num_x_channels, ych))
            );

        // Update filters
        // main_state->H_hat and shadow_state->H_hat are updated.
        PAR_JOBS(
            PJOB(filter_adapt_task, (tdist.par_2_tasks[0], main_state, shadow_state, AEC_2_TASKS_PASSES, ych)),
            PJOB(filter_adapt_task, (tdist.par_2_tasks[1], main_state, shadow_state, AEC_2_TASKS_PASSES, ych))
            );
    }
    framenum++; 
}

void calc_time_domain_ema_energy_task(par_tasks_and_channels_t* s, aec_state_t *state, int32_t *output, int passes, int channels, enum e_td_ema type) {
    for(int i=0; i<passes; i++) {
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(type == Y_EMA) {
                aec_calc_time_domain_ema_energy(&state->shared_state->y_ema_energy[ch], &state->shared_state->y[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
            }
            else if(type == X_EMA) {
                aec_calc_time_domain_ema_energy(&state->shared_state->x_ema_energy[ch], &state->shared_state->x[ch], AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
            }
            else if(type == ERROR_EMA) {
                int32_t (*ptr)[AEC_FRAME_ADVANCE] = (int32_t(*)[AEC_FRAME_ADVANCE])output;
                //create a bfp_s32_t structure to point to output array
                bfp_s32_t temp;
                bfp_s32_init(&temp, &ptr[ch][0], -31, AEC_FRAME_ADVANCE, 1);
                
                aec_calc_time_domain_ema_energy(&state->error_ema_energy[ch], &temp, 0, AEC_FRAME_ADVANCE, &state->shared_state->config_params);
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
                aec_forward_fft(
                        &main_state->shared_state->Y[ch],
                        &main_state->shared_state->y[ch]);
            }
            else if(type == X_FFT) {
                aec_forward_fft(
                        &main_state->shared_state->X[ch],
                        &main_state->shared_state->x[ch]);
            }
            else if((type==ERROR_FFT) && (task==0)) {
                aec_forward_fft(
                        &main_state->Error[ch],
                        &main_state->error[ch]
                        ); //error -> Error
            }
            else if((type==ERROR_FFT) && (task==1) && (shadow_state != NULL)){
                aec_forward_fft(
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
                aec_calc_X_fifo_energy(main_state, ch, recalc_bin);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_X_fifo_energy(shadow_state, ch, recalc_bin);
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
                aec_inverse_fft(&main_state->error[ch], &main_state->Error[ch]);
            }
            else if(task == 1){
                aec_inverse_fft(&main_state->y_hat[ch], &main_state->Y_hat[ch]);
            }
            else {
                if(shadow_state != NULL) {
                    aec_inverse_fft(&shadow_state->error[ch], &shadow_state->Error[ch]);
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

void calc_output_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int32_t *output_main, int32_t *output_shadow, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                int32_t (*tmp)[AEC_FRAME_ADVANCE] = (int32_t(*)[AEC_FRAME_ADVANCE])output_main;
                aec_calc_output(main_state, &tmp[ch], ch);
            }
            else {
                if(shadow_state != NULL) {
                    if(output_shadow != NULL) {
                        int32_t (*tmp)[AEC_FRAME_ADVANCE] = (int32_t(*)[AEC_FRAME_ADVANCE])output_shadow;
                        aec_calc_output(shadow_state, &tmp[ch], ch);
                    }
                    else {
                        aec_calc_output(shadow_state, NULL, ch);
                    }
                }
            }
        }
    }
}

void calc_freq_domain_energy_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels)
{
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_calc_freq_domain_energy(&main_state->overall_Error[ch], &main_state->Error[ch]);
            }
            else if(task == 1){
                aec_calc_freq_domain_energy(&main_state->shared_state->overall_Y[ch], &main_state->shared_state->Y[ch]);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_freq_domain_energy(&shadow_state->overall_Error[ch], &shadow_state->Error[ch]);
                }
            }
        }
    }
}

void calc_normalisation_spectrum_task(par_tasks_and_channels_t *s, aec_state_t *main_state, aec_state_t *shadow_state, int passes, int channels) {
    for(int i=0; i<passes; i++) {
        int task = s[i].task;
        int ch = s[i].channel;
        if(ch >= channels) continue;
        int is_active = s[i].is_active;
        if(is_active) {
            if(task == 0) {
                aec_calc_normalisation_spectrum(main_state, ch, 0);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_normalisation_spectrum(shadow_state, ch, 1);
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
                aec_calc_T(main_state, ych, xch);
            }
            else {
                if(shadow_state != NULL) {
                    aec_calc_T(shadow_state, ych, xch);
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
