// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "vnr_features_api.h"
#include "vnr_features_priv.h"

void vnr_input_state_init(vnr_input_state_t *input_state) {
    memset(input_state, 0, sizeof(vnr_input_state_t));
}

void vnr_form_input_frame(vnr_input_state_t *input_state, bfp_complex_s32_t *X, int32_t *input_frame, const int32_t *new_x_frame) {
    // Update current frame
    memcpy(input_frame, input_state->prev_input_samples, (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
    memcpy(&input_frame[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE], new_x_frame, VNR_FRAME_ADVANCE*sizeof(int32_t));

    // Update previous samples
    memcpy(input_state->prev_input_samples, &input_frame[VNR_FRAME_ADVANCE], (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));

    vnr_priv_forward_fft(X, input_frame);
}

void vnr_feature_state_init(vnr_feature_state_t *feature_state) {
    memset(feature_state, 0, sizeof(vnr_feature_state_t));
}

void vnr_extract_features(vnr_feature_state_t *vnr_feature_state, bfp_s32_t *feature_patch, int32_t *feature_patch_data, const bfp_complex_s32_t *X) {
    fixed_s32_t new_slice[VNR_MEL_FILTERS];
    vnr_priv_make_slice(new_slice, X);
    vnr_priv_add_new_slice(vnr_feature_state->feature_buffers, new_slice);
    vnr_priv_normalise_patch(feature_patch, feature_patch_data, (const vnr_feature_state_t*)vnr_feature_state);
}
