// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>
#include <print.h>

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


void pipeline_init(pipeline_state_t *state, aec_conf_t *aec_conf) {
    memset(state, 0, sizeof(pipeline_state_t)); 
    
    // AEC init
    aec_init(&state->aec_main_state, &state->aec_shadow_state, &state->aec_shared_state,
            &state->aec_main_memory_pool[0], &state->aec_shadow_memory_pool[0],
            aec_conf->num_y_channels, aec_conf->num_x_channels,
            aec_conf->num_main_filt_phases, aec_conf->num_shadow_filt_phases);

    //AGC init
    
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
}
