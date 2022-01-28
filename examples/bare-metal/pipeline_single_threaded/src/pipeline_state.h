#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "aec_state.h"
#include "aec_memory_pool.h"
#include "agc_api.h"

typedef struct {
    // AEC
    aec_state_t DWORD_ALIGNED aec_main_state;
    aec_state_t DWORD_ALIGNED aec_shadow_state;
    aec_shared_state_t DWORD_ALIGNED aec_shared_state;
    uint8_t DWORD_ALIGNED aec_main_memory_pool[sizeof(aec_memory_pool_t)];
    uint8_t DWORD_ALIGNED aec_shadow_memory_pool[sizeof(aec_shadow_filt_memory_pool_t)];

    // AGC
    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
} pipeline_state_t;

#endif
