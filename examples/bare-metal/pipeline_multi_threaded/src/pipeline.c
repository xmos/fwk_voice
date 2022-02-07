// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include <stdlib.h>
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/channel_transaction.h>
#include <xcore/port.h>
#include <xcore/parallel.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>

#include "pipeline_config.h"
#include "pipeline_state.h"
#include "stage_1.h"

#include "suppression.h"
#include "agc_api.h"

extern void aec_process_frame_2threads(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE],
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE]);

DECLARE_JOB(pipeline_stage_1, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_2, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_3, (chanend_t, chanend_t));

/// pipeline_stage_1
// Stage 1 state
stage_1_state_t DWORD_ALIGNED stage_1_state;
void pipeline_stage_1(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;

    aec_conf_t aec_de_mode_conf, aec_non_de_mode_conf;
    aec_non_de_mode_conf.num_y_channels = AP_MAX_Y_CHANNELS;
    aec_non_de_mode_conf.num_x_channels = AP_MAX_X_CHANNELS;
    aec_non_de_mode_conf.num_main_filt_phases = AEC_MAIN_FILTER_PHASES;
    aec_non_de_mode_conf.num_shadow_filt_phases = AEC_SHADOW_FILTER_PHASES;

    aec_de_mode_conf.num_y_channels = 1;
    aec_de_mode_conf.num_x_channels = 1;
    aec_de_mode_conf.num_main_filt_phases = 30;
    aec_de_mode_conf.num_shadow_filt_phases = 0;
    
    adec_config_t adec_conf = ADEC_CONFIG_DE_ONLY_AT_STARTUP;
    
    stage_1_init(&stage_1_state, &aec_de_mode_conf, &aec_non_de_mode_conf, &adec_conf);
    int32_t DWORD_ALIGNED frame[AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED stage_1_out[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], ((AP_MAX_X_CHANNELS+AP_MAX_Y_CHANNELS) * AP_FRAME_ADVANCE));
        
        // AEC, DE ADEC
        stage_1_process_frame(&stage_1_state, &stage_1_out[0], &md.max_ref_energy, &md.aec_corr_factor[0], &frame[0], &frame[AP_MAX_Y_CHANNELS]);

        // Transmit metadata
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));

        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&stage_1_out[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 

    }
}

/// pipeline_stage_2
void pipeline_stage_2(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;
    //Initialise NS
    sup_state_t DWORD_ALIGNED sup_state[AP_MAX_Y_CHANNELS];
    for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
        sup_init_state(&sup_state[ch]);
    }

    int32_t DWORD_ALIGNED frame [AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1){
        // Receive and bypass metadata
        chan_in_buf_byte(c_frame_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));

        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        /**NS*/
        for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
            //the frame buffer will be used for both input and output here
            sup_process_frame(&sup_state[ch], frame[ch], frame[ch]);
        }

        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
    }
}


/// pipeline_stage_3
void pipeline_stage_3(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;
    // Initialise AGC
    agc_config_t agc_conf_asr = AGC_PROFILE_ASR;
    agc_config_t agc_conf_comms = AGC_PROFILE_COMMS;
    agc_conf_asr.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.adapt_on_vad = 0; // We don't have VAD yet

    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
    agc_init(&agc_state[0], &agc_conf_asr);
    for(int ch=1; ch<AP_MAX_Y_CHANNELS; ch++) {
        agc_init(&agc_state[ch], &agc_conf_comms);
    }

    agc_meta_data_t agc_md;
    agc_md.vad_flag = AGC_META_DATA_NO_VAD;

    int32_t frame[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive metadata
        chan_in_buf_byte(c_frame_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        agc_md.aec_ref_power = md.max_ref_energy;

        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        /** AGC*/
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            agc_md.aec_corr_factor = md.aec_corr_factor[ch];
            // Memory optimisation: Reuse input memory for AGC output
            agc_process_frame(&agc_state[ch], frame[ch], frame[ch], &agc_md);
        }

        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
    }
}


/// Pipeline
void pipeline(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a) {
    // 3 stage pipeline. stage 1: AEC, stage 2: NS, stage 3: AGC
    channel_t c_stage_1_to_2 = chan_alloc();
    channel_t c_stage_2_to_3 = chan_alloc();
    
    PAR_JOBS(
        PJOB(pipeline_stage_1, (c_pcm_in_b, c_stage_1_to_2.end_a)),
        PJOB(pipeline_stage_2, (c_stage_1_to_2.end_b, c_stage_2_to_3.end_a)),
        PJOB(pipeline_stage_3, (c_stage_2_to_3.end_b, c_pcm_out_a))
    );
}
