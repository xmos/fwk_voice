// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>
#include <print.h>

#include "aec_defines.h"
#include "aec_api.h"
#include "de_api.h"
#include "adec_api.h"

#include "aec_config.h"
#include "aec_task_distribution.h"

#include "pipeline_state.h"

#if PROFILE_PROCESSING
#include "profile.h"
#endif

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


// After aec_init, these values are overwritten to modify convergence behaviour
//https://github.com/xmos/lib_aec/pull/190
//const fixed_s32_t fixed_mu_delay_est_mode = (int)(0.4 * (1<<30)); //TODO this will probably need tuning. 0.4 is the value that works with delay_est tests in lib_aec


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
    int num_channels = (delay_state->delay_samples) > 0 ? AP_MAX_Y_CHANNELS : AP_MAX_X_CHANNELS;
    if (delay_state->delay_samples >= 0) {/** Requested Mic delay +ve => delay mic*/
        for(int ch=0; ch<num_channels; ch++) {
            for(int i=0; i<AP_FRAME_ADVANCE; i++) {
                get_delayed_sample(delay_state, &input_y_data[ch][i], ch);
            }
        }
    }
    else if (delay_state->delay_samples < 0) {/* Requested Mic delay negative => advance mic which can't be done, so delay reference*/
        for(int ch=0; ch<num_channels; ch++) {
            for(int i=0; i<AP_FRAME_ADVANCE; i++) {
                get_delayed_sample(delay_state, &input_x_data[ch][i], ch);
            }
        }
    }
    return;
}

void aec_switch_configuration(pipeline_state_t *state, aec_conf_t *conf)
{
    aec_init(&state->aec_main_state, &state->aec_shadow_state, &state->aec_shared_state,
            &state->aec_main_memory_pool[0], &state->aec_shadow_memory_pool[0],
            conf->num_y_channels, conf->num_x_channels,
            conf->num_main_filt_phases, conf->num_shadow_filt_phases);
}

/*def is_frame_active(new_frame, threshold):
    abs_max_ref = abs(np.max(new_frame))
    return abs_max_ref > threshold*/
int32_t is_frame_active(pipeline_state_t *state, int32_t (*input_x_data)[AP_FRAME_ADVANCE], int32_t num_x_channels) {
    bfp_s32_t ref;
    int32_t ref_active_flag = 0; 
    for(int ch=0; ch<num_x_channels; ch++) {
        bfp_s32_init(&ref, &input_x_data[ch][0], -31, AP_FRAME_ADVANCE, 1);
        float_s32_t max = bfp_s32_max(&ref);
        max = float_s32_abs(max);
        ref_active_flag = ref_active_flag | (float_s32_gt(max, state->ref_active_threshold));
    }
    return ref_active_flag;
}


void reset_all_aec(pipeline_state_t *state){
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

void pipeline_init(pipeline_state_t *state, aec_conf_t *de_conf, aec_conf_t *non_de_conf) {
    memset(state, 0, sizeof(pipeline_state_t)); 
    prof(0, "start_pipeline_init"); //Start profiling after memset since the pipeline components do the memset as part of their init functions.
    state->delay_estimator_enabled = 0;
    state->adec_requested_delay_samples = 0;
    state->ref_active_threshold =  double_to_float_s32(pow(10, -60/20.0));

    // Initialise default delay values
    delay_buffer_init(&state->delay_state, 0/*Initialise with 0 delay_samples*/);
    
    memcpy(&state->aec_de_mode_conf, de_conf, sizeof(aec_conf_t));
    memcpy(&state->aec_non_de_mode_conf, non_de_conf, sizeof(aec_conf_t));

    adec_init(&state->adec_state);
#if DELAY_ESTIMATION_ENABLED_ON_STARTUP
    // If DE is enabled only on startup, trigger delay estimation cycle once
    state->adec_state.adec_config.force_de_cycle_trigger = 1;
#endif
    aec_switch_configuration(state, &state->aec_non_de_mode_conf);
    prof(1, "end_pipeline_init");
}

int framenum = 0;
void pipeline_process_frame(pipeline_state_t *state,
        int32_t (*input_y_data)[AP_FRAME_ADVANCE],
        int32_t (*input_x_data)[AP_FRAME_ADVANCE],
        int32_t (*output_data)[AP_FRAME_ADVANCE])
{
    framenum++;
    /** Get delayed frame*/
    prof(2, "start_get_delayed_frame");
    delay_state_t *delay_state_ptr = &state->delay_state;
    get_delayed_frame(
            input_y_data,
            input_x_data,
            delay_state_ptr
            );
    prof(3, "end_get_delayed_frame");
    
    prof(4, "start_is_frame_active");
    // Calculate far_end active
    int is_ref_active = is_frame_active(state, input_x_data, state->aec_main_state.shared_state->num_x_channels);
    prof(5, "end_is_frame_active");

    //printf("frame %d\n",framenum);

    /** AEC*/
    prof(6, "start_aec_process_frame");
    int32_t aec_output_shadow[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    // Writing main filter output to output_data directly

#if (AEC_THREAD_COUNT == 1)
        aec_process_frame_1thread(&state->aec_main_state, &state->aec_shadow_state, input_y_data, input_x_data, output_data, aec_output_shadow);
#elif (AEC_THREAD_COUNT == 2)
        aec_process_frame_2threads(&state->aec_main_state, &state->aec_shadow_state, input_y_data, input_x_data, output_data, aec_output_shadow);
#else
        #error "C app only supported for AEC_THREAD_COUNT range [1, 2]"
#endif
    prof(7, "end_aec_process_frame");
    
    prof(8, "start_estimate_delay");
    /** Delay estimator*/
    de_output_t de_output;
    estimate_delay(
            &de_output,
            state->aec_main_state.H_hat[0],
            state->aec_main_state.num_phases
            );
    
    prof(9, "end_estimate_delay");

    prof(10, "start_adec_process_frame");
    /** ADEC*/
    // Create input to ADEC
    adec_input_t adec_in;
    // From DE
    adec_in.from_de.delay_estimate = de_output.measured_delay;
    adec_in.from_de.peak_power_phase_index = de_output.peak_power_phase_index;
    adec_in.from_de.peak_phase_power = de_output.peak_phase_power;
    adec_in.from_de.peak_to_average_ratio = de_output.peak_to_average_ratio;
    // From AEC
    adec_in.from_aec.y_ema_energy_ch0 = state->aec_main_state.shared_state->y_ema_energy[0];
    adec_in.from_aec.error_ema_energy_ch0 = state->aec_main_state.error_ema_energy[0];
    adec_in.from_aec.shadow_flag = state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0];
    adec_in.from_aec.shadow_better_or_equal_flag = 0;
    if(state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0] > EQUAL) {
        adec_in.from_aec.shadow_better_or_equal_flag = 1;
    }
    adec_in.from_aec.shadow_to_main_copy_flag = 0;
    if(state->aec_main_state.shared_state->shadow_filter_params.shadow_flag[0] == COPY) {
        adec_in.from_aec.shadow_to_main_copy_flag = 1;
    }
    // Directly from app
    adec_in.far_end_active_flag = is_ref_active;
    adec_in.num_frames_since_last_call = 1;
     
    
    // Log current mode for printing later
    adec_mode_t old_mode = state->adec_state.mode;
    
    // Call ADEC
    adec_output_t adec_output;
    adec_process_frame(
            &state->adec_state,
            &adec_output,
            &adec_in
            );
    
    prof(11, "end_adec_process_frame");

    prof(12, "start_reset_aec");
    //** Reset AEC state if needed*/
    if(adec_output.reset_all_aec_flag) {
        reset_all_aec(state);
    }
    prof(13, "end_reset_aec");
    
    prof(14, "start_update_delay_buffer");
    /** Update delay samples if there's a delay change requested by ADEC*/
    if(adec_output.delay_change_request_flag == 1){
        // In case the mode change is requested as a result of force DE cycle trigger, reset force_de_cycle_trigger
        state->adec_state.adec_config.force_de_cycle_trigger = 0;

        // Update delay_buffer delay_samples with mic delay requested by adec
        state->delay_state.delay_samples = adec_output.requested_mic_delay_samples;
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            reset_partial_delay_buffer(&state->delay_state, ch, state->delay_state.delay_samples);
        }
#ifdef ENABLE_ADEC_DEBUG_PRINTS
        printf("!!ADEC STATE CHANGE!! frame %d, old: %s new: %s\n", framenum, old_mode?"DE":"AEC", state->adec_state.mode?"DE":"AEC");

        printf("AP Setting MIC delay to: %ld\n", state->delay_state.delay_samples);
#endif
    }
    prof(15, "end_update_delay_buffer");
    
    prof(16, "start_overwrite_output_in_de_mode");
    /** Overwrite output with mic input if delay estimation enabled*/
    if (state->delay_estimator_enabled) {
        // Send the current frame unprocessed
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            for(int i=0; i<AP_FRAME_ADVANCE; i++) {
                output_data[ch][i] = input_y_data[ch][i];
            }
        }
    }
    prof(17, "end_overwrite_output_in_de_mode");

    prof(18, "start_switch_aec_config");
    /* If a delay change is requested and this request is not accompanied by a AEC -> DE change, update
     * state->adec_requested_delay_samples(bits [0:30]) and toggle last bit (bit31) of
     * state->adec_requested_delay_samples to indicate to the test that a delay change event has occured*/
    if((adec_output.delay_change_request_flag == 1) && (adec_output.delay_estimator_enabled_flag == 0)){
        /* Update MSB 31 bits of state->adec_requested_delay_samples with the new delay. Toggle LSB bit of
         * state->adec_requested_delay_samples to indicate that a new delay (even if it happens to be same as the
         * existing delay) is requested by adec*/
         unsigned new_bottom_bit = (state->adec_requested_delay_samples & 0x1) ^ 0x1; 
         state->adec_requested_delay_samples = adec_output.requested_delay_samples_debug;
         state->adec_requested_delay_samples = (state->adec_requested_delay_samples & 0xfffffffe) | new_bottom_bit;
    }

    /** Switch AEC config if needed*/
    if (adec_output.delay_estimator_enabled_flag && !state->delay_estimator_enabled) {
        // Initialise AEC for delay estimation config
        aec_switch_configuration(state, &state->aec_de_mode_conf);
        state->aec_main_state.shared_state->config_params.coh_mu_conf.adaption_config = AEC_ADAPTION_FORCE_ON;
        //state->aec_main_state.shared_state->config_params.coh_mu_conf.force_adaption_mu_q30 = fixed_mu_delay_est_mode;
        state->delay_estimator_enabled = 1;
    } else if ((!adec_output.delay_estimator_enabled_flag && state->delay_estimator_enabled)) {
        // Start AEC for normal aec config
        aec_switch_configuration(state, &state->aec_non_de_mode_conf);
        state->delay_estimator_enabled = 0;
    }
    prof(19, "end_switch_aec_config");

    print_prof(0, 20, framenum);
}
