#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "stage_1.h"
#include "sup_state.h"
#include "agc_api.h"

typedef struct {
    // Stage1 - AEC, DE, ADEC
    stage_1_state_t DWORD_ALIGNED stage_1_state;
    // NS
    sup_state_t DWORD_ALIGNED sup_state[AP_MAX_Y_CHANNELS];
    // AGC
    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
} pipeline_state_t;

#endif
