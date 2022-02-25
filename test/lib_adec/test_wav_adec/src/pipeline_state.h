#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "aec_api.h"
#include "adec_state.h"

#include "aec_memory_pool.h"
#include "delay_buffer.h"

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
    uint8_t num_y_channels;
    uint8_t num_x_channels;
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
    delay_buf_state_t delay_state;

    //Top level
    aec_conf_t aec_de_mode_conf;
    aec_conf_t aec_non_de_mode_conf;
    int32_t delay_estimator_enabled;
    int32_t adec_requested_delay_samples; // Delay requested from ADEC in case of a delay change event
    float_s32_t ref_active_threshold; //-60dB
    float_s32_t peak_to_average_ratio; //for logging
    int32_t adec_output_delay_estimator_enabled_flag; // to keep persistant across frames
    int32_t de_output_measured_delay_samples; //for logging in test_wav
} pipeline_state_t;

#if !PROFILE_PROCESSING
    #define prof(n, str)
    #define print_prof(start, end, framenum)
#endif

#endif
