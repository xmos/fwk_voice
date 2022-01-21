// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>

#include "aec_api.h"
#include "pipeline_config.h"
#include "pipeline_state.h"

extern void aec_process_frame_1thread(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE],
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE]);


void pipeline_init(pipeline_state_t *state) {
    memset(state, 0, sizeof(pipeline_state_t)); 
    
    // Initialise AEC
    aec_init(&state->aec_main_state, &state->aec_shadow_state, &state->aec_shared_state,
            &state->aec_main_memory_pool[0], &state->aec_shadow_memory_pool[0],
            AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS,
            AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES);

    // Initialise AGC
    agc_config_t agc_conf_asr = AGC_PROFILE_ASR;
    agc_config_t agc_conf_comms = AGC_PROFILE_COMMS;
    agc_conf_asr.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.lc_enabled = 1; // Enable loss control on comms
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
    /** AEC*/
    int32_t aec_output_shadow[AEC_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t aec_output_main[AEC_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];

    aec_process_frame_1thread(&state->aec_main_state, &state->aec_shadow_state, input_y_data, input_x_data, aec_output_main, aec_output_shadow);
    
    agc_meta_data_t agc_md;
    agc_md.aec_ref_power = aec_calc_max_ref_energy(input_x_data, AP_MAX_X_CHANNELS);
    agc_md.vad_flag = AGC_META_DATA_NO_VAD;
    
    /** AGC*/
    for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
        agc_md.aec_corr_factor = aec_calc_corr_factor(&state->aec_main_state, ch);
        agc_process_frame(&state->agc_state[ch], output_data[ch], aec_output_main[ch], &agc_md);
    }
}
