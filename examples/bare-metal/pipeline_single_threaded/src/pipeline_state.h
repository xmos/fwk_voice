#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "stage_1.h"
#include "ic_state.h"
#include "vad_api.h"
#include "ns_state.h"
#include "agc_api.h"

typedef struct {
    // Stage1 - AEC, DE, ADEC
    stage_1_state_t DWORD_ALIGNED stage_1_state;
    // IC, VAD
    ic_state_t DWORD_ALIGNED ic_state;
    vad_state_t DWORD_ALIGNED vad_state;
    // NS
    ns_state_t DWORD_ALIGNED ns_state[AP_MAX_Y_CHANNELS];
    // AGC
    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
} pipeline_state_t;

#endif
