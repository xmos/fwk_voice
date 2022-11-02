#ifndef AP_STAGE_A_STATE_H
#define AP_STAGE_A_STATE_H

#include "pipeline_config.h"
#include "xmath/xmath.h"

typedef struct {
    float_s32_t max_ref_energy;
    float_s32_t aec_corr_factor[AP_MAX_Y_CHANNELS];
    int32_t ref_active_flag;
    int32_t vnr_pred_flag;
}pipeline_metadata_t;

#endif
