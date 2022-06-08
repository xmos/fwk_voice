#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_input_state_init(vnr_input_state_t *input_state);

void vnr_form_input_frame(vnr_input_state_t *input_state,
        bfp_complex_s32_t *X,
        complex_s32_t X_data[VNR_FD_FRAME_LENGTH],
        const int32_t new_x_frame[VNR_FRAME_ADVANCE]);

void vnr_feature_state_init(vnr_feature_state_t *feature_state);

void vnr_extract_features(vnr_feature_state_t *vnr_feature_state,
        bfp_s32_t *feature_patch,
        int32_t feature_patch_data[VNR_PATCH_WIDTH * VNR_MEL_FILTERS],
        const bfp_complex_s32_t *X);

#endif
