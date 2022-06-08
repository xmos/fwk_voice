// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "vnr_features_api.h"
#include "vnr_inference_state.h"

extern void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_ie_config_t *ie_config);
extern void vnr_priv_init_ie_config(vnr_ie_config_t *ie_config);

static vnr_input_state_t vnr_input_state;
static vnr_feature_state_t vnr_feature_state;
static vnr_ie_config_t vnr_ie_config;
void test_init()
{
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);

    //TODO Initializing vnr_ie_config locally here and not calling vnr_inference_init() to avoid linking with avona::vnr::inference which doesn't compile for x86
    vnr_priv_init_ie_config(&vnr_ie_config);
}

void test(int32_t *output, int32_t *input)
{
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr_input_state, &X, input_frame, input);

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

    memcpy(output, &feature_patch.exp, sizeof(int32_t));
    memcpy(&output[1], feature_patch.data, feature_patch.length * sizeof(int32_t));
    
    // Testing normalised feature as well as quantised feature generation in this test
    int8_t quantised_patch[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_priv_feature_quantise(quantised_patch, &feature_patch, &vnr_ie_config);

    memcpy(&output[1+feature_patch.length], quantised_patch, VNR_PATCH_WIDTH*VNR_MEL_FILTERS*sizeof(int8_t));
}
