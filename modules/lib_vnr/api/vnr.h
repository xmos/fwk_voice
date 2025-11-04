#pragma once
#include <stdint.h>

#include "xmath/xmath.h"

#include "vnr_features_api.h"
#include "vnr_inference_api.h"

typedef struct{
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    int32_t *input_frame;
    float_s32_t vnr_output_s32;
    float vnr_output_float32;
} vnr_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

void vnr_init(vnr_ctx_t *ctx);
void vnr_add_frame(vnr_ctx_t *ctx, int32_t new_frame[VNR_FRAME_ADVANCE]);
void vnr_compute(vnr_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
