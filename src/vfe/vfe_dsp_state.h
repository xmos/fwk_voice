// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef VFE_STAGE_STATE_H_
#define VFE_STAGE_STATE_H_

#include <limits.h>

#include "voice_toolbox.h"
#include "agc_conf.h"
#include "agc.h"
#include "suppression_conf.h"
#include "suppression.h"
#include "ic_conf.h"
#include "interference_canceller.h"
#include "vad.h"

#define STAGE_1_X_DELAY  (180)
#define STAGE_1_MD_BUFFER_SIZE ((STAGE_1_X_DELAY + ((1<<IC_PROC_FRAME_LENGTH_LOG2) % IC_FRAME_ADVANCE) + IC_FRAME_ADVANCE/2) / IC_FRAME_ADVANCE)

#define STAGE_2_RX_CHANNEL_PAIRS 1
#define STAGE_2_DUPLICATE_CH_INDEX 0

#define COMMS_FILT_NUM_SECTIONS 2
#define COMMS_FILT_NUM_COEFFS (COMMS_FILT_NUM_SECTIONS * DSP_NUM_COEFFS_PER_BIQUAD)
#define COMMS_FILT_STATE (COMMS_FILT_NUM_SECTIONS * DSP_NUM_STATES_PER_BIQUAD)
#define COMMS_FILT_Q_FORMAT (28)

#define VFE_PROC_FRAME_LENGTH   (1 << (VFE_PROC_FRAME_LENGTH_LOG2))

typedef struct {
    ic_state_t ic_state;
    vad_state_t vad_state;

    uint32_t smoothed_voice_chance;
    uint32_t voice_chance_alpha;

    vtb_u32_float_t input_energy;
    vtb_u32_float_t output_energy;
    uint32_t energy_alpha;

    vtb_u32_float_t input_energy0;
    vtb_u32_float_t output_energy0;
    uint32_t energy_alpha0;

    int32_t vad_data_window[VFE_PROC_FRAME_LENGTH];

    int enable_filter_instability_recovery;
    //This defines the ratio of the output to input at which the filter will reset.
    //A good rules is it should be above 2.0
    vtb_u32_float_t out_to_in_ratio_limit;

    vtb_q0_31_t instability_recovery_leakage_alpha;

    vtb_md_t md_buffer[STAGE_1_MD_BUFFER_SIZE];
    uint32_t md_buffer_index;

    uint64_t alignment_padding;
    // Used for assembling VFE_PROC_FRAME_LENGTH audio frames
    vtb_ch_pair_t proc_frame[VFE_CHANNEL_PAIRS][VFE_PROC_FRAME_LENGTH];
    vtb_ch_pair_t prev_frame[VFE_CHANNEL_PAIRS][VFE_PROC_FRAME_LENGTH - VFE_FRAME_ADVANCE];
} vfe_dsp_stage_1_state_t;

typedef struct {
    uint64_t alignment_padding_0;
    int32_t filtState[COMMS_FILT_STATE];

    agc_state_t agc_state;
    uint32_t agc_bypass;

    uint64_t alignment_padding_1;
    // Used for assembling VFE_PROC_FRAME_LENGTH audio frames
    vtb_ch_pair_t proc_frame[VFE_CHANNEL_PAIRS][VFE_PROC_FRAME_LENGTH];
    vtb_ch_pair_t prev_frame[VFE_CHANNEL_PAIRS][VFE_PROC_FRAME_LENGTH - VFE_FRAME_ADVANCE];

    //Note the ordering of this is carefully chosen so that it
    //works around http://bugzilla/show_bug.cgi?id=14893
    //Because SUP_X_CHANNELS may = 0 in sup_state
    uint64_t alignment_padding_2;
    suppression_state_t sup_state;
    //Note padding here to account for the 4Byte entries that XC
    //makes for zero length arrays, whereas C does not. Otherwise
    //XC may overshoot the C structure during memset(sizeof) etc.
    //es_state has 3 potentially zero length arrays and we have
    //potentially 2 y channels of that in suppression_state.
    //suppression_state also has 2 additional arrays (above).
    uint32_t padding_protection[3 * SUP_Y_CHANNELS + 2];
} vfe_dsp_stage_2_state_t;

#endif
