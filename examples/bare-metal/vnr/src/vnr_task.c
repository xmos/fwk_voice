// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include "vnr_features_api.h"
#include "vnr_inference_api.h"
#if PROFILE_PROCESSING
#include "profile.h"
#else
static void prof(int n, const char* str) {}
static void print_prof(int a, int b, int framenum){}
#endif

void vnr(chanend_t c_frame_in, chanend_t c_frame_out)
{
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    
    prof(0, "start_vnr_init");
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init();
    prof(1, "end_vnr_init");
    if(err) {
        printf("ERROR: vnr_inference_init() returned error %ld\n",err);
        assert(0);
    }

    int framenum = 0;
    while(1)
    {
        int32_t new_frame[240];
        chan_in_buf_word(c_frame_in, (uint32_t*)&new_frame[0], 240);

        prof(2, "start_vnr_form_input_frame");
        complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);
        prof(3, "end_vnr_form_input_frame");

        prof(4, "start_vnr_extract_features");
        bfp_s32_t feature_patch;
        int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
        vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);
        prof(5, "end_vnr_extract_features");

        prof(6, "start_vnr_inference");
        float_s32_t inference_output;
        //printf("inference_output: 0x%x, %d\n", inference_output.mant, inference_output.exp);
        vnr_inference(&inference_output, &feature_patch);
        prof(7, "end_vnr_inference");
        chan_out_buf_byte(c_frame_out, (uint8_t*)&inference_output, sizeof(float_s32_t));
        
        framenum++;
        print_prof(0, 8, framenum);        
    }
}
