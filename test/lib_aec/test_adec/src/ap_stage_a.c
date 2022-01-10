// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>
#include <print.h>

#include "aec_defines.h"
#include "aec_api.h"
#include "aec_delay_estimator_control.h"

#include "aec_config.h"
#include "aec_task_distribution.h"

extern void aec_process_frame_1thread(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE],
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE]);

extern void aec_process_frame_2threads(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE],
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE]);

#include "ap_stage_a_state.h"

// After aec_init, these values are overwritten to modify convergence behaviour
//https://github.com/xmos/lib_aec/pull/190
const fixed_s32_t fixed_mu_delay_est_mode = (int)(0.4 * (1<<30)); //TODO this will probably need tuning. 0.4 is the value that works with delay_est tests in lib_aec


void delay_buffer_init(delay_state_t *state, int default_delay_samples) {
    memset(state->delay_buffer, 0, sizeof(state->delay_buffer));
    memset(&state->curr_idx[0], 0, sizeof(state->curr_idx));
    state->delay_samples = default_delay_samples;
}

void get_delayed_sample(delay_state_t *delay_state, int32_t *sample, int32_t ch) {
    delay_state->delay_buffer[ch][delay_state->curr_idx[ch]] = *sample;
    int32_t abs_delay_samples = (delay_state->delay_samples < 0) ? -delay_state->delay_samples : delay_state->delay_samples;
    // Send back the samples with the correct delay
    uint32_t delay_idx = (
            (MAX_DELAY_SAMPLES + delay_state->curr_idx[ch] - abs_delay_samples)
            % MAX_DELAY_SAMPLES
            );
    *sample = delay_state->delay_buffer[ch][delay_idx];
    delay_state->curr_idx[ch] = (delay_state->curr_idx[ch] + 1) % MAX_DELAY_SAMPLES;
}

void reset_partial_delay_buffer(delay_state_t *delay_state, int32_t ch, int32_t num_samples) {
    num_samples = (num_samples < 0) ? -num_samples : num_samples;
    //Reset num_samples samples before curr_idx
    int32_t reset_start = (
            (MAX_DELAY_SAMPLES + delay_state->curr_idx[ch] - num_samples)
            % MAX_DELAY_SAMPLES
            );
    if(reset_start < delay_state->curr_idx[ch]) {
        //reset_start hasn't wrapped around
        memset(&delay_state->delay_buffer[ch][reset_start], 0, num_samples*sizeof(int32_t));
    }
    else {
        //reset_start has wrapped around
        memset(&delay_state->delay_buffer[ch][0], 0, delay_state->curr_idx[ch]*sizeof(int32_t));
        int remaining = num_samples - delay_state->curr_idx[ch];
        memset(&delay_state->delay_buffer[ch][MAX_DELAY_SAMPLES - remaining], 0, remaining*sizeof(int32_t));
    }
}


static inline void get_delayed_frame(
        int32_t (*input_y_data)[AP_FRAME_ADVANCE],
        int32_t (*input_x_data)[AP_FRAME_ADVANCE],
        delay_state_t *delay_state)
{    
    for(int i=0; i<AP_FRAME_ADVANCE; i++) {
        for(int ch=0; ch<2; ch++) {
            if (delay_state->delay_samples > 0) {
                /* Requested Mic delay +ve => delay mic*/
                get_delayed_sample(delay_state, &input_y_data[ch][i], ch);
            }
            else if (delay_state->delay_samples < 0) {
                /* Requested Mic delay negative => advance mic which can't be done => delay reference*/
                get_delayed_sample(delay_state, &input_x_data[ch][i], ch);
            }
        }
    }
    return;
}

void aec_switch_configuration(ap_stage_a_state *state, aec_conf_t *conf)
{
    aec_init(&state->aec_main_state, &state->aec_shadow_state, &state->aec_shared_state,
            &state->aec_main_memory_pool[0], &state->aec_shadow_memory_pool[0],
            conf->num_y_channels, conf->num_x_channels,
            conf->num_main_filt_phases, conf->num_shadow_filt_phases);
}



void reset_all_aec(ap_stage_a_state *state){
    aec_state_t *main_state = &state->aec_main_state;
    aec_state_t *shadow_state = &state->aec_shadow_state;
    aec_shared_state_t *shared_state = (aec_shared_state_t*)state->aec_main_state.shared_state; 
    int y_channels = shared_state->num_y_channels;
    int x_channels = shared_state->num_x_channels;
    int main_phases = main_state->num_phases;
    int shadow_phases = shadow_state->num_phases;
    //Main H_hat
    for(int ch=0; ch<y_channels; ch++) {
        for(int ph=0; ph<x_channels*main_phases; ph++) {
            //memset(main_state->H_hat[ch][ph].data, 0, main_state->H_hat[ch][ph].length*sizeof(complex_s32_t));
            main_state->H_hat[ch][ph].exp = -1024;
            main_state->H_hat[ch][ph].hr = 31;
        }
    }
    //Shadow H_hat
    for(int ch=0; ch<y_channels; ch++) {
        for(int ph=0; ph<x_channels*shadow_phases; ph++) {
            //memset(shadow_state->H_hat[ch][ph].data, 0, shadow_state->H_hat[ch][ph].length*sizeof(complex_s32_t));
            shadow_state->H_hat[ch][ph].exp = -1024;
            shadow_state->H_hat[ch][ph].hr = 31;
        }
    }
    //X_fifo
    for(int ch=0; ch<x_channels; ch++) {
        for(int ph=0; ph<main_phases; ph++) {
            shared_state->X_fifo[ch][ph].exp = -1024;
            shared_state->X_fifo[ch][ph].hr = 31;
        }
    }
    //X_energy, sigma_XX
    for(int ch=0; ch<x_channels; ch++) {
        main_state->X_energy[ch].exp = -1024;
        main_state->X_energy[ch].hr = 31;
        shadow_state->X_energy[ch].exp = -1024;
        shadow_state->X_energy[ch].hr = 31;
        shared_state->sigma_XX[ch].exp = -1024;
        shared_state->sigma_XX[ch].hr = 31;
    }
}

void ap_stage_a_init(ap_stage_a_state *state) {
    memset(state, 0, sizeof(ap_stage_a_state));
    state->delay_estimator_enabled = 0;
    state->wait_for_initial_adec = INITIAL_DELAY_ESTIMATION;

    // Initialise default delay values
    delay_buffer_init(&state->delay_state, 0/*Initialise with 0 delay_samples*/);
    memset(&state->adec_output, 0, sizeof(adec_output_t));

    state->delay_conf.num_x_channels = 1;
    state->delay_conf.num_y_channels = 1;
    state->delay_conf.num_main_filt_phases = 30;
    state->delay_conf.num_shadow_filt_phases = 0;

    state->run_conf_alt_arch.num_x_channels = 2; //ALT_ARCH_AEC_X_CHANNELS;   // Far
    state->run_conf_alt_arch.num_y_channels = 1; //ALT_ARCH_AEC_Y_CHANNELS;   // Mics
    state->run_conf_alt_arch.num_main_filt_phases = 15; //ALT_ARCH_AEC_PHASES;
    state->run_conf_alt_arch.num_shadow_filt_phases = 5; //ALT_ARCH_AEC_SHADOW_FILTER_PHASES;

    adec_init(&state->adec_state);
    aec_conf_t *conf = &state->run_conf_alt_arch;
    aec_switch_configuration(state, conf);
}

int framenum = 0;
void ap_stage_a(ap_stage_a_state *state,
        int32_t (*input_y_data)[AP_FRAME_ADVANCE],
        int32_t (*input_x_data)[AP_FRAME_ADVANCE],
        int32_t (*output_data)[AP_FRAME_ADVANCE])
{
    unsigned rx_elapsed;
    // Rx mic and ref audio
    delay_state_t *delay_state_ptr = &state->delay_state;
    get_delayed_frame(
            input_y_data,
            input_x_data,
            delay_state_ptr
            );

    if (state->adec_output.delay_estimator_enabled && !state->delay_estimator_enabled) {
        // Init delay estimator
        aec_conf_t *conf = &state->delay_conf;
        aec_switch_configuration(state, conf);
        //Set adaption to AEC_ADAPTION_FORCE_ON
        state->aec_main_state.shared_state->config_params.coh_mu_conf.adaption_config = AEC_ADAPTION_FORCE_ON;
        state->aec_main_state.shared_state->config_params.coh_mu_conf.force_adaption_mu_q30 = fixed_mu_delay_est_mode;
        state->delay_estimator_enabled = 1;

        // if delay estimation has just been finished or AEC reset has been triggered, init AEC,
        // note that if aec_reset_timeout is negative, AEC reset triggering is not enabled
    } else if ((!state->adec_output.delay_estimator_enabled && state->delay_estimator_enabled)) {
        // Init AEC
        aec_conf_t *conf = &state->run_conf_alt_arch;
        aec_switch_configuration(state, conf);

        state->delay_estimator_enabled = 0;
    }

    //TODO Port vtb_is_frame_active() later
    int is_ref_active = 1;
    /*for (unsigned x_ch=0; x_ch < state->aec_state.conf.num_x_channels; ++x_ch) {
      is_ref_active |= vtb_is_frame_active(curr_frame_ref, AP_FRAME_ADVANCE, x_ch, AP_SWITCH_ACTIVITY_REF_THRESHOLD);
      }*/

    //printf("frame %d\n",framenum);
    int32_t aec_output_main[2][240];
    int32_t aec_output_shadow[2][240];

#if (AEC_THREAD_COUNT == 1)
        aec_process_frame_1thread(&state->aec_main_state, &state->aec_shadow_state, input_y_data, input_x_data, aec_output_main, aec_output_shadow);
#elif (AEC_THREAD_COUNT == 2)
        aec_process_frame_2threads(&state->aec_main_state, &state->aec_shadow_state, input_y_data, input_x_data, aec_output_main, aec_output_shadow);
#else
        #error "C app only supported for AEC_THREAD_COUNT range [1, 2]"
#endif

    int delay_estimate = aec_estimate_delay(
            &state->aec_main_state.shared_state->delay_estimator_params,
            state->aec_main_state.H_hat[0],
            state->aec_main_state.num_phases
            );

    //Convert to de to adec input format
    delay_estimator_params_t *de_params = (delay_estimator_params_t *)&state->aec_main_state.shared_state->delay_estimator_params;
    de_to_adec_t de_to_adec;
    de_to_adec.delay_estimate = delay_estimate;
    de_to_adec.peak_power_phase_index = de_params->peak_power_phase_index;
    //peak_phase_power
    de_to_adec.peak_phase_power = de_params->peak_phase_power;
    //peak_to_average_ratio
    de_to_adec.peak_to_average_ratio = de_params->peak_to_average_ratio;
    //erle_ratio
    aec_to_adec_t aec_to_adec;
    aec_to_adec.y_ema_energy_ch0 = state->aec_main_state.shared_state->y_ema_energy[0];
    aec_to_adec.error_ema_energy_ch0 = state->aec_main_state.error_ema_energy[0];
    aec_to_adec.shadow_better_or_equal_flag = 0;
    if(state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0] > EQUAL) {
        aec_to_adec.shadow_better_or_equal_flag = 1;
    }
    aec_to_adec.shadow_to_main_copy_flag = 0;
    if(state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0] == COPY) {
        aec_to_adec.shadow_to_main_copy_flag = 1;
    }
    //printf("erle_ratio %f, shadow_flag %d\n", ldexp(((double)(uint32_t)aec_to_adec.erle_ratio.m), aec_to_adec.erle_ratio.e), state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0]);
    framenum++;

    adec_mode_t old_mode = state->adec_state.mode;

    adec_process_frame(
            &state->adec_state,
            &state->adec_output,
            &de_to_adec,
            &aec_to_adec,
            is_ref_active,
            1 //ADEC called every frame so one frame since last call
            );

    if(state->adec_output.reset_all_aec_flag) {
        reset_all_aec(state);
    }

    if(state->adec_output.mode_change_request_flag == 1){
        // Update delay_buffer delay_samples with mic delay requested by adec
        state->delay_state.delay_samples = state->adec_output.requested_mic_delay_samples;
        for(int ch=0; ch<2; ch++) {
            reset_partial_delay_buffer(&state->delay_state, ch, state->delay_state.delay_samples);
        }
        state->wait_for_initial_adec = 0;
        printf("!!ADEC STATE CHANGE!!  old: %s new: %s\n", old_mode?"DE":"AEC", state->adec_state.mode?"DE":"AEC");

        printf("AP Setting MIC delay to: %d\n", state->delay_state.delay_samples);
    }
    if (state->delay_estimator_enabled) {
        // Send the current frame unprocessed
        for(int ch=0; ch<2; ch++) {
            for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
                output_data[ch][i] = input_y_data[ch][i];
            }
        }

    } else {
        for(int ch=0; ch<2; ch++) {
            for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
                output_data[ch][i] = aec_output_main[ch][i];
            }
        }
    }
}
