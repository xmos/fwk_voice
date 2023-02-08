// Copyright 2022 XMOS LIMITED.
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
#include "ic_api.h"
#include "ns_api.h"
#include "agc_api.h"

#define VNR_AGC_THRESHOLD (0.5)
#define PRINT_VNR_PREDICTION (0)

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
DECLARE_JOB(pipeline_stage_4, (chanend_t, chanend_t));

/// pipeline_stage_1
// Stage 1 state
stage_1_state_t DWORD_ALIGNED stage_1_state;
void pipeline_stage_1(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;

    aec_conf_t aec_de_mode_conf, aec_non_de_mode_conf;
#if ALT_ARCH_MODE
    aec_non_de_mode_conf.num_y_channels = 1;
    aec_non_de_mode_conf.num_x_channels = AP_MAX_X_CHANNELS;
    aec_non_de_mode_conf.num_main_filt_phases = 15;
    aec_non_de_mode_conf.num_shadow_filt_phases = AEC_SHADOW_FILTER_PHASES;
#else
    aec_non_de_mode_conf.num_y_channels = AP_MAX_Y_CHANNELS;
    aec_non_de_mode_conf.num_x_channels = AP_MAX_X_CHANNELS;
    aec_non_de_mode_conf.num_main_filt_phases = AEC_MAIN_FILTER_PHASES;
    aec_non_de_mode_conf.num_shadow_filt_phases = AEC_SHADOW_FILTER_PHASES;
#endif

    aec_de_mode_conf.num_y_channels = 1;
    aec_de_mode_conf.num_x_channels = 1;
    aec_de_mode_conf.num_main_filt_phases = 30;
    aec_de_mode_conf.num_shadow_filt_phases = 0;
    
    // Disable ADEC's automatic mode. We only want to estimate and correct for the delay at startup
    adec_config_t adec_conf;
    adec_conf.bypass = 1; // Bypass automatic DE correction
#if DISABLE_INITIAL_DELAY_EST
    adec_conf.force_de_cycle_trigger = 0; // Do not force a DE correction cycle ob startup
#else
    adec_conf.force_de_cycle_trigger = 1; // Force a delay correction cycle, so that delay correction happens once after initialisation. Make sure this is set back to 0 after adec has requested a transition into DE mode once, to stop any further delay correction (automatic or forced) by ADEC
#endif
    stage_1_init(&stage_1_state, &aec_de_mode_conf, &aec_non_de_mode_conf, &adec_conf);

    int32_t DWORD_ALIGNED frame[AP_MAX_X_CHANNELS + AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED stage_1_out[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE]; // stage1 will not process the frame in-place since Mic input is needed to overwrite the output in certain cases
    while(1) {
        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], ((AP_MAX_X_CHANNELS+AP_MAX_Y_CHANNELS) * AP_FRAME_ADVANCE));
#if DISABLE_STAGE_1
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
        continue;
#endif
        
        // AEC, DE ADEC
        stage_1_process_frame(&stage_1_state, &stage_1_out[0], &md.max_ref_energy, &md.aec_corr_factor[0], &md.ref_active_flag, &frame[0], &frame[AP_MAX_Y_CHANNELS]);
        // If AEC has processed fewer y channels than downstream stages (in DE mode for example), then copy aec_corr_factor[0] to other channels
        if(stage_1_state.aec_main_state.shared_state->num_y_channels < AP_MAX_Y_CHANNELS) {
            for(int ch=stage_1_state.aec_main_state.shared_state->num_y_channels; ch<AP_MAX_Y_CHANNELS; ch++) {
                md.aec_corr_factor[ch] = md.aec_corr_factor[0];
            }
        }
        
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
    // Initialise IC and VNR
    ic_state_t DWORD_ALIGNED ic_state;
    float_s32_t input_vnr_pred, output_vnr_pred;
    float_s32_t agc_vnr_threshold = f32_to_float_s32(VNR_AGC_THRESHOLD);
    ic_init(&ic_state);

    int32_t DWORD_ALIGNED frame[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive metadata
        chan_in_buf_byte(c_frame_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
#if DISABLE_STAGE_2
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
        continue;
#endif
        
#if ALT_ARCH_MODE
        if(md.ref_active_flag) {
            ic_state.config_params.bypass = 1;
        }
        else {
            ic_state.config_params.bypass = 0;
        }
#endif
        /** IC*/
        // Calculating the ASR channel
        ic_filter(&ic_state, frame[0], frame[1], frame[0]);
        // VNR
        ic_calc_vnr_pred(&ic_state, &input_vnr_pred, &output_vnr_pred);
#if PRINT_VNR_PREDICTION 
        printf("VNR OUTPUT PRED: %ld %d\n", output_vnr_pred.mant, output_vnr_pred.exp);
        printf("VNR INPUT PRED: %ld %d\n", input_vnr_pred.mant, input_vnr_pred.exp);
#endif
        md.vnr_pred_flag = float_s32_gt(output_vnr_pred, agc_vnr_threshold);

        // Transferring metadata
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));

        // Adapting the IC
        ic_adapt(&ic_state, input_vnr_pred);

        // Copy IC output to the other channel
        for(int v = 0; v < AP_FRAME_ADVANCE; v++){
            frame[1][v] = frame[0][v];
        }

        // Transferring output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
    }
}

/// pipeline_stage_3
void pipeline_stage_3(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;
    // Initialise NS
    ns_state_t DWORD_ALIGNED ns_state[AP_MAX_Y_CHANNELS];
    for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
        ns_init(&ns_state[ch]);
    }

    int32_t DWORD_ALIGNED frame [AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1){
        // Receive and bypass metadata
        chan_in_buf_byte(c_frame_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
#if DISABLE_STAGE_3
        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
        continue;
#endif

        chan_out_buf_byte(c_frame_out, (uint8_t*)&md, sizeof(pipeline_metadata_t));

        /** NS*/
        for(int ch = 0; ch < AP_MAX_Y_CHANNELS; ch++){
            // The frame buffer will be used for both input and output here
            ns_process_frame(&ns_state[ch], frame[ch], frame[ch]);
        }

        // Transmit output frame
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
    }
}


/// pipeline_stage_4
void pipeline_stage_4(chanend_t c_frame_in, chanend_t c_frame_out) {
    // Pipeline metadata
    pipeline_metadata_t md;
    // Initialise AGC
    agc_config_t agc_conf_asr = AGC_PROFILE_ASR;
#if DISABLE_AGC_ADAPT_GAIN
    agc_conf_asr.adapt = 0;
#endif

    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
    agc_init(&agc_state[0], &agc_conf_asr);
    agc_init(&agc_state[1], &agc_conf_asr);

    agc_meta_data_t agc_md;

    int32_t frame[AP_MAX_Y_CHANNELS][AP_FRAME_ADVANCE];
    while(1) {
        // Receive metadata
        chan_in_buf_byte(c_frame_in, (uint8_t*)&md, sizeof(pipeline_metadata_t));
        agc_md.aec_ref_power = md.max_ref_energy;
        agc_md.vnr_flag = md.vnr_pred_flag;

        // Receive input frame
        chan_in_buf_word(c_frame_in, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE));
#if DISABLE_STAGE_4
        chan_out_buf_word(c_frame_out, (uint32_t*)&frame[0][0], (AP_MAX_Y_CHANNELS * AP_FRAME_ADVANCE)); 
        continue;
#endif

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
// 3 stage pipeline. stage 1: AEC, stage 2: IC and VNR, stage 3: NS, stage 4: AGC
void pipeline_tile0(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a) {
    
    pipeline_stage_1(c_pcm_in_b, c_pcm_out_a);
}

void pipeline_tile1(chanend_t c_pcm_in_b, chanend_t c_pcm_out_a)
{
    channel_t c_stage_2_to_3 = chan_alloc();
    channel_t c_stage_3_to_4 = chan_alloc();
    PAR_JOBS(
        PJOB(pipeline_stage_2, (c_pcm_in_b, c_stage_2_to_3.end_a)),
        PJOB(pipeline_stage_3, (c_stage_2_to_3.end_b, c_stage_3_to_4.end_a)),
        PJOB(pipeline_stage_4, (c_stage_3_to_4.end_b, c_pcm_out_a))
    );
}
