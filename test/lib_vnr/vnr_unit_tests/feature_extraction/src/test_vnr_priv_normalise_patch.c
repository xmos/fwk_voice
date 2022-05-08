// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr_features_api.h"
#include "vnr_features_priv.h"

static vnr_feature_state_t vnr_feature_state;
void test_init() {
    vnr_feature_state_init(&vnr_feature_state);
}

void test(int32_t *output, int32_t *input) {
    vnr_priv_add_new_slice(vnr_feature_state.feature_buffers, input); 

    bfp_s32_t normalised_patch;
    int32_t normalised_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_priv_normalise_patch(&normalised_patch, normalised_patch_data, &vnr_feature_state);

    memcpy(output, &normalised_patch.exp, sizeof(int32_t));
    memcpy(&output[1], normalised_patch.data, normalised_patch.length * sizeof(int32_t));
}
