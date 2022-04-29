// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include "vnr_features_api.h"
#include "vnr_inference_api.h"

void vnr(chanend_t c_frame_in, chanend_t c_frame_out)
{
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    vnr_ie_state_t vnr_ie_state;

    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init(&vnr_ie_state);
    if(err) {
        printf("ERROR: vnr_inference_init() returned error %ld\n",err);
        assert(0);
    }
    while(1)
    {
        int32_t new_frame[240];
        chan_in_buf_word(c_frame_in, (uint32_t*)&new_frame[0], 240);

        int32_t DWORD_ALIGNED input_frame[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);

        bfp_s32_t feature_patch;
        int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
        vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

        float_s32_t inference_output;
        //printf("inference_output: 0x%x, %d\n", inference_output.mant, inference_output.exp);
        vnr_inference(&vnr_ie_state, &inference_output, &feature_patch);
        chan_out_buf_byte(c_frame_out, (uint8_t*)&inference_output, sizeof(float_s32_t));
    }
}
