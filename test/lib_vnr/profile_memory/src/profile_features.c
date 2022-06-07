
// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vnr_features_api.h"

vnr_input_state_t vnr_input_state;
vnr_feature_state_t vnr_feature_state;

int main(int argc, char** argv) {    
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);

    int32_t new_frame[VNR_FRAME_ADVANCE];
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

    return feature_patch_data[0];
}
