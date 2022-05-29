#include <stdio.h>
#include <string.h>
#include "vnr_inference_api.h"

int32_t wrapper_vnr_inference_init(vnr_ie_state_t *ie_ptr) {
    int err = vnr_inference_init(ie_ptr);
    return err;
}

void wrapper_vnr_inference(vnr_ie_state_t *ie_state, float_s32_t *vnr_output, bfp_s32_t *features) {
    vnr_inference(ie_state, vnr_output, features);
}

