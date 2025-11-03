#pragma once
#include <stdint.h>

#include "xmath/xmath.h"

#include "vnr_features_api.h"
#include "vnr_inference_api.h"

typedef struct{
    // States
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    // Input
    int32_t new_frame[VNR_FRAME_ADVANCE];
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    // Output
    float_s32_t vnr_output_s32;
    float vnr_output_float32;
} vnr_ctx_t;

// Note: user should use ctx->new_frame to provide new input frame data

#ifdef __cplusplus
extern "C" {
#endif

void vnr_simple_init(vnr_ctx_t *ctx);
void vnr_simple_compute(vnr_ctx_t *ctx);

#ifdef __cplusplus
}
#endif
