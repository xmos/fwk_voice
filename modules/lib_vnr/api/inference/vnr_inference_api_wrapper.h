
#ifndef __VNR_INFERENCE_API_WRAPPER_H__
#define __VNR_INFERENCE_API_WRAPPER_H__

#include "vnr_inference_state.h"

int32_t wrapper_vnr_inference_init(vnr_ie_state_t *ie_ptr);
void wrapper_vnr_inference(vnr_ie_state_t *ie_state, float_s32_t *vnr_output, bfp_s32_t *features);

#endif
