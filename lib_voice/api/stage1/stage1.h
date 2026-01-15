#ifndef STAGE1_STATE_H
#define STAGE1_STATE_H

#include "aec.h"
#include "adec.h"
#include "delay_buffer.h"

#define REF_ACTIVE_THRESHOLD_dB (-60) // Reference input level above which it is considered active
#define HOLD_AEC_LIMIT_SECONDS (3) // Keep AEC enabled for atleast 3seconds after detecting reference as inactive. Used only in alt arch configuration

typedef struct {
    uint8_t num_x_channels;
    uint8_t num_y_channels;
    uint8_t num_main_filt_phases;
    uint8_t num_shadow_filt_phases;
    const aec_task_distribution_t * tdist;
} aec_conf_t;

typedef struct {
    // AEC
    aec_state_t DWORD_ALIGNED aec_state;

    // ADEC
    adec_state_t DWORD_ALIGNED adec_state;

    // Delay Buffer
    delay_buf_state_t DWORD_ALIGNED delay_state;

    //Top level
    aec_conf_t aec_de_mode_conf;
    aec_conf_t aec_non_de_mode_conf;
    int32_t delay_estimator_enabled;
    float_s32_t ref_active_threshold; //-60dB
    //alt-arch
    int32_t hold_aec_count;
    int32_t hold_aec_limit;
} stage1_t;

void stage1_init(stage1_t *state, aec_conf_t *de_conf, aec_conf_t *non_de_conf, adec_config_t *adec_config);

void stage1_process_frame(stage1_t *state, int32_t (*output_frame)[AEC_FRAME_ADVANCE],
    float_s32_t *max_ref_energy, float_s32_t *aec_corr_factor, int32_t *ref_active_flag,
    int32_t (*input_y)[AEC_FRAME_ADVANCE], int32_t (*input_x)[AEC_FRAME_ADVANCE]);
#endif
