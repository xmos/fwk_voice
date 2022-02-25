// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>

#include "aec_api.h"
#include "ic_api.h"
#include "vad_api.h"
#include "ns_api.h"
#include "pipeline_config.h"
#include "pipeline_state.h"
#include "stage_1.h"

extern void aec_process_frame_1thread(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE],
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE]);


void pipeline_init(pipeline_state_t *state) {
    memset(state, 0, sizeof(pipeline_state_t)); 
    
    // Initialise AEC, DE, ADEC stage
    aec_conf_t aec_de_mode_conf, aec_non_de_mode_conf;
    aec_non_de_mode_conf.num_y_channels = AP_MAX_Y_CHANNELS;
    aec_non_de_mode_conf.num_x_channels = AP_MAX_X_CHANNELS;
    aec_non_de_mode_conf.num_main_filt_phases = AEC_MAIN_FILTER_PHASES;
    aec_non_de_mode_conf.num_shadow_filt_phases = AEC_SHADOW_FILTER_PHASES;

    aec_de_mode_conf.num_y_channels = 1;
    aec_de_mode_conf.num_x_channels = 1;
    aec_de_mode_conf.num_main_filt_phases = 30;
    aec_de_mode_conf.num_shadow_filt_phases = 0;
    
    // Disable ADEC's automatic mode. We only want to estimate and correct for the delay at startup
    adec_config_t adec_conf;
    adec_conf.bypass = 1; //Bypass automatic DE correction
    adec_conf.force_de_cycle_trigger = 1; //Force a delay correction cycle, so that delay correction happens once after initialisation. Make sure this is set back to 0 after adec has requested a transition into DE mode once, to stop any further delay correction (automatic or forced) by ADEC

    stage_1_init(&state->stage_1_state, &aec_de_mode_conf, &aec_non_de_mode_conf, &adec_conf);

    ic_init(&state->ic_state);
    vad_init(&state->vad_state);

    // Initialise NS
    for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
        ns_init(&state->ns_state[ch]);
    }
    
    // Initialise AGC
    agc_config_t agc_conf_asr = AGC_PROFILE_ASR;
    agc_config_t agc_conf_comms = AGC_PROFILE_COMMS;
    agc_conf_asr.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.adapt_on_vad = 0; // We don't have VAD yet
    agc_init(&state->agc_state[0], &agc_conf_asr);
    for(int ch=1; ch<AP_MAX_Y_CHANNELS; ch++) {
        agc_init(&state->agc_state[ch], &agc_conf_comms);
    }
}

void pipeline_process_frame(pipeline_state_t *state,
        int32_t (*input_y_data)[AP_FRAME_ADVANCE],
        int32_t (*input_x_data)[AP_FRAME_ADVANCE],
        int32_t (*output_data)[AP_FRAME_ADVANCE])
{
    /** Stage1 - AEC, DE, ADEC*/
    int32_t stage_1_out[AEC_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    float_s32_t max_ref_energy, aec_corr_factor[AEC_MAX_Y_CHANNELS];
    stage_1_process_frame(&state->stage_1_state, &stage_1_out[0], &max_ref_energy, &aec_corr_factor[0], input_y_data, input_x_data);

    /**IC and VAD*/
    int32_t ic_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    //The ASR channel will be produced by IC filtering
    ic_filter(&state->ic_state, stage_1_out[0], stage_1_out[1], ic_output[0]);
    uint8_t vad = vad_probability_voice(ic_output[0], &state->vad_state);
    ic_adapt(&state->ic_state, vad, ic_output[0]);
    //The comms channel will be produced by two channels averaging
    for(int v = 0; v < AP_FRAME_ADVANCE; v++){
        ic_output[1][v] = (stage_1_out[0][v] >> 1) + (stage_1_out[1][v] >> 1);
    }

    /**NS*/
    int32_t ns_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
        ns_process_frame(&state->ns_state[ch], ns_output[ch], ic_output[ch]);
    }
    
    /** AGC*/
    agc_meta_data_t agc_md;
    agc_md.aec_ref_power = max_ref_energy;
    agc_md.vad_flag = (vad > 205);

    for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
        agc_md.aec_corr_factor = aec_corr_factor[ch];
        agc_process_frame(&state->agc_state[ch], output_data[ch], ns_output[ch], &agc_md);
    }
}
