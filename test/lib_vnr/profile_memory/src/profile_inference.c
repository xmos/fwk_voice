// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vnr_inference_api.h"

int main(int argc, char** argv) {    
    vnr_inference_init();
    
    float_s32_t inference_output;
    bfp_s32_t feature_patch;
    int32_t feature_patch_data[4*24];
    bfp_s32_init(&feature_patch, feature_patch_data, 0, 4*24, 0);

    vnr_inference(&inference_output, &feature_patch);
    return feature_patch.data[0];
}
