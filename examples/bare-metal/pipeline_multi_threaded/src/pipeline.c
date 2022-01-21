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

DECLARE_JOB(pipeline_stage_1, (chanend_t, chanend_t));
DECLARE_JOB(pipeline_stage_2, (chanend_t, chanend_t));


/// pipeline_stage_1
void pipeline_stage_1(chanend_t c_frame_in, chanend_t c_frame_out) {
    // AEC
    aec_state_t DWORD_ALIGNED aec_main_state;
    aec_state_t DWORD_ALIGNED aec_shadow_state;
    aec_shared_state_t DWORD_ALIGNED aec_shared_state;
    uint8_t DWORD_ALIGNED aec_main_memory_pool[sizeof(aec_memory_pool_t)];
    uint8_t DWORD_ALIGNED aec_shadow_memory_pool[sizeof(aec_shadow_filt_memory_pool_t)];
    
    // Initialise AEC
    aec_init(&aec_main_state, &aec_shadow_state, &aec_shared_state,
            &aec_main_memory_pool[0], &aec_shadow_memory_pool[0],
            AP_MAX_Y_CHANNELS, AP_MAX_X_CHANNELS,
            AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES);

    int32_t DWORD_ALIGNED frame[AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t aec_output_shadow[AEC_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t aec_output_main[AEC_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], ((AP_MAX_X_CHANNELS+AP_MAX_Y_CHANNELS) * AP_FRAME_ADVANCE));

        /** AEC*/
        aec_process_frame_1thread(&aec_main_state, &aec_shadow_state, &frame[0], &frame[AP_MAX_Y_CHANNELS], aec_output_main, aec_output_shadow);
        
        /** Metadata*/
        float_s32_t ref_power = aec_calc_max_ref_energy(&frame[AP_MAX_Y_CHANNELS], AP_MAX_X_CHANNELS);
        float_s32_t aec_corr_factor[AP_MAX_Y_CHANNELS];
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            aec_corr_factor[ch] = aec_calc_corr_factor(&aec_main_state, ch);
        }
        // Transmit stage 1 metadata
        float_s32_t stage_1_metadata[1 + AP_MAX_Y_CHANNELS];
        stage_1_metadata[0] = ref_power;
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            stage_1_metadata[1+ch] = aec_corr_factor[ch];
        }
        chan_out_buf_byte(c_frame_out, (uint8_t*)stage_1_metadata, (1 + AP_MAX_Y_CHANNELS)*sizeof(float_s32_t));


        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&aec_output_main[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
    }
}


/// pipeline_stage_2
void pipeline_stage_2(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Initialise AGC
    agc_config_t agc_conf_asr = AGC_PROFILE_ASR;
    agc_config_t agc_conf_comms = AGC_PROFILE_COMMS;
    agc_conf_asr.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.adapt_on_vad = 0; // We don't have VAD yet
    agc_conf_comms.lc_enabled = 1; // Enable loss control on comms

    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
    agc_init(&agc_state[0], &agc_conf_asr);
    for(int ch=1; ch<AP_MAX_Y_CHANNELS; ch++) {
        agc_init(&agc_state[ch], &agc_conf_comms);
    }

    agc_meta_data_t agc_md;
    agc_md.vad_flag = AGC_META_DATA_NO_VAD;

    int32_t DWORD_ALIGNED frame[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t agc_output[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive metadata from stage 1
        float_s32_t stage_1_metadata[1 + AP_MAX_Y_CHANNELS];
        chan_in_buf_byte(c_frame_in, (uint8_t*)stage_1_metadata, (1 + AP_MAX_Y_CHANNELS)*sizeof(float_s32_t));
        agc_md.aec_ref_power = stage_1_metadata[0];

        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));

        /** AGC*/
        for(int ch=0; ch<AP_MAX_Y_CHANNELS; ch++) {
            agc_md.aec_corr_factor = stage_1_metadata[1+ch];
            agc_process_frame(&agc_state[ch], agc_output[ch], frame[ch], &agc_md);
        }

        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&agc_output[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
    }
}


/// Pipeline
void pipeline(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a) {
    // 2 stage pipeline. stage 1: AEC, stage 2: AGC
    channel_t c_stage_1_to_2 = chan_alloc();
    
    PAR_JOBS(
        PJOB(pipeline_stage_1, (c_pcm_in_b, c_stage_1_to_2.end_a)),
        PJOB(pipeline_stage_2, (c_stage_1_to_2.end_b, c_pcm_out_a))
    );
}
