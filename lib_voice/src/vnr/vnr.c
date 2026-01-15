// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "vnr.h"

void vnr_state_init(vnr_state_t *vnr)
{
    vnr_input_state_init(&vnr->input_state);
    vnr_feature_state_init(&vnr->feature_state);
    int32_t ret = vnr_inference_init();
    if(ret) {
        printf("vnr_inference_init() returned error %ld\n",ret);
        assert(0);
    }
}

void vnr_process_frame(vnr_state_t * vnr, float_s32_t * output, int32_t input[VNR_FRAME_ADVANCE])
{
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr->input_state, &X, input_frame, input);

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_extract_features(&vnr->feature_state, &feature_patch, feature_patch_data, &X);

    vnr_inference(output, &feature_patch);
}
