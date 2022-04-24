// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "vnr_features_api.h"
#include "vnr_features_priv.h"

void vnr_feature_state_init(vnr_feature_state_t *feature_state) {
    memset(feature_state, 0, sizeof(vnr_feature_state_t));
}

void vnr_form_input_frame(vnr_feature_state_t *feature_state, bfp_complex_s32_t *X, const int32_t *new_x_frame) {
    // Update current frame
    memcpy(feature_state->scratch_data, feature_state->prev_input_samples, (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
    memcpy(&feature_state->scratch_data[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE], new_x_frame, VNR_FRAME_ADVANCE*sizeof(int32_t));

    // Update previous samples
    memcpy(feature_state->prev_input_samples, &feature_state->scratch_data[VNR_FRAME_ADVANCE], (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));

    vnr_priv_forward_fft(X, feature_state->scratch_data);
}


fixed_s32_t* vnr_make_slice(vnr_feature_state_t *feature_state, bfp_complex_s32_t *X) {
    // MEL
    float_s32_t mel_output[VNR_MEL_FILTERS];
    
    vnr_priv_mel_compute(mel_output, X);

    vnr_priv_log2(feature_state->scratch_data, mel_output, VNR_MEL_FILTERS); //Calculate new_slice in state->scratch_data
    return (fixed_s32_t*)feature_state->scratch_data;
}

void vnr_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice)
{
    for(unsigned index=0; index<VNR_PATCH_WIDTH - 1; index++) {
        memcpy(feature_buffers[index], feature_buffers[index+1], VNR_MEL_FILTERS*sizeof(int32_t));
    }
    memcpy(feature_buffers[VNR_PATCH_WIDTH - 1], new_slice, VNR_MEL_FILTERS*sizeof(int32_t));
}

void vnr_normalise_patch(vnr_feature_state_t *feature_state, bfp_s32_t *normalised_patch)
{
#if (VNR_FD_FRAME_LENGTH < (VNR_MEL_FILTERS*VNR_PATCH_WIDTH))
    #error ERROR squared_mag_data memory not enough for reuse as normalised_patch
#endif
    bfp_s32_init(normalised_patch, feature_state->scratch_data, 0, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    //norm_patch = feature_patch - np.max(feature_patch)   
    bfp_s32_t feature_patch_bfp;
    bfp_s32_init(&feature_patch_bfp, (int32_t*)&feature_state->feature_buffers[0][0], VNR_LOG2_OUTPUT_EXP, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    float_s32_t max = bfp_s32_max(&feature_patch_bfp);
    float_s32_t zero = {0, 0};
    float_s32_t neg_max = float_s32_sub(zero, max);
    bfp_s32_add_scalar(normalised_patch, &feature_patch_bfp, neg_max);
    /*printf("framenum %d\n",framenum);
    printf("max = %.6f\n",ldexp(max.mant, max.exp));
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/

}
