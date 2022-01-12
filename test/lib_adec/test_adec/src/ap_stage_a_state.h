#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "aec_api.h"
#include "adec_state.h"

#include "aec_memory_pool.h"

#define AP_FRAME_ADVANCE           (240)
#define AP_MAX_Y_CHANNELS (2)
#define AP_MAX_X_CHANNELS (2)
#define AP_MAX_CHANNELS ((AP_MAX_Y_CHANNELS > AP_MAX_X_CHANNELS) ? (AP_MAX_Y_CHANNELS) : (AP_MAX_X_CHANNELS) )

#define MAX_DELAY_MS                ( 150 )
#define MAX_DELAY_SAMPLES           ( 16000*MAX_DELAY_MS/1000 )

#ifndef INITIAL_DELAY_ESTIMATION
#define INITIAL_DELAY_ESTIMATION (0)
#endif

typedef struct {
    // Circular buffer to store the samples
    int32_t delay_buffer[AP_MAX_CHANNELS][MAX_DELAY_SAMPLES];
    // index of the value for the samples to be stored in the buffer
    int32_t curr_idx[AP_MAX_CHANNELS];
    int32_t delay_samples;
} delay_state_t;

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
    
    // ADEC
    adec_state_t DWORD_ALIGNED adec_state;
 
    // Delay Buffer
    delay_state_t delay_state;

    //Top level
    aec_conf_t delay_conf;
    aec_conf_t run_conf_alt_arch;
    int32_t delay_estimator_enabled;
    int32_t adec_requested_delay_samples; // Delay requested from ADEC in case of a delay change event
} ap_stage_a_state;

#endif
