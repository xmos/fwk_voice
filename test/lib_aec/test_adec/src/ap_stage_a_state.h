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
    int32_t delay_buffer[2][MAX_DELAY_SAMPLES];
    // index of the value for the samples to be stored in the buffer
    uint32_t curr_idx[2];
    int32_t delay_samples;
} delay_state_t;

typedef struct {
    uint8_t num_x_channels;
    uint8_t num_y_channels;
    uint8_t num_main_filt_phases;
    uint8_t num_shadow_filt_phases;
} aec_conf_t;

typedef struct {
    aec_state_t aec_main_state;
    uint64_t align1;
    aec_state_t aec_shadow_state;
    uint64_t align2;
    aec_shared_state_t aec_shared_state;
    uint64_t align3;
    uint8_t aec_main_memory_pool[sizeof(aec_memory_pool_t)];
    uint64_t align4;
    uint8_t aec_shadow_memory_pool[sizeof(aec_shadow_filt_memory_pool_t)];

    uint64_t align5;
    adec_state_t adec_state;

    delay_state_t delay_state;
    adec_output_t adec_output;
    aec_conf_t delay_conf;
    aec_conf_t run_conf_alt_arch;
    int32_t delay_estimator_enabled;
    uint64_t alignment;
} ap_stage_a_state;

void ap_stage_a(ap_stage_a_state *state,
    int32_t (*input_y_data)[AP_FRAME_ADVANCE],
    int32_t (*input_x_data)[AP_FRAME_ADVANCE],
    int32_t (*output_data)[AP_FRAME_ADVANCE]);

#endif
