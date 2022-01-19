#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "aec_state.h"
#include "aec_memory_pool.h"

typedef struct {
    uint8_t num_x_channels;
    uint8_t num_y_channels;
    uint8_t num_main_filt_phases;
    uint8_t num_shadow_filt_phases;
} aec_conf_t;

typedef struct {
    // AEC
    aec_state_t DWORD_ALIGNED aec_main_state;
    aec_state_t DWORD_ALIGNED aec_shadow_state;
    aec_shared_state_t DWORD_ALIGNED aec_shared_state;
    uint8_t DWORD_ALIGNED aec_main_memory_pool[sizeof(aec_memory_pool_t)];
    uint8_t DWORD_ALIGNED aec_shadow_memory_pool[sizeof(aec_shadow_filt_memory_pool_t)];

    // AGC
} pipeline_state_t;

#endif
