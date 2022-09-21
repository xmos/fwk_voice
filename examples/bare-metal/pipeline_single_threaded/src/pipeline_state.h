#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "stage_1.h"
#include "ic_state.h"
#include "ns_state.h"
#include "agc_api.h"

typedef struct {
    float_s32_t max_ref_energy;
    float_s32_t aec_corr_factor[AP_MAX_Y_CHANNELS];
    int32_t ref_active_flag;
    int32_t vnr_pred_flag;
}pipeline_metadata_t;

typedef struct {
    // Stage1 - AEC, DE, ADEC
    stage_1_state_t DWORD_ALIGNED stage_1_state;
} pipeline_state_tile0_t;

typedef struct {
    // IC, VNR
    ic_state_t DWORD_ALIGNED ic_state;
    float_s32_t input_vnr_pred, output_vnr_pred;
    // NS
    ns_state_t DWORD_ALIGNED ns_state[AP_MAX_Y_CHANNELS];
    // AGC
    agc_state_t agc_state[AP_MAX_Y_CHANNELS];
} pipeline_state_tile1_t;

#endif
