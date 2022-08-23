#ifndef _CALC_VNR_PRED_H_
#define _CALC_VNR_PRED_H_

#include "vnr_features_api.h"
#include "vnr_inference_api.h"
#include "bfp_math.h"

typedef struct {
    vnr_feature_state_t feature_state[2];
    float_s32_t input_vnr_pred;
    float_s32_t output_vnr_pred;
    fixed_s32_t pred_alpha_q30;
}vnr_pred_state_t;

void calc_vnr_pred(
    vnr_pred_state_t *vnr_state,
    const bfp_complex_s32_t *Y,
    const bfp_complex_s32_t *Error
    );

void init_vnr_pred_state(vnr_pred_state_t *vnr_pred_state);
#endif

