#ifndef __VNR_INFERENCE_API_H__
#define __VNR_INFERENCE_API_H__

#include "vnr_inference_state.h"

#ifdef __cplusplus
extern "C" {
#endif
    #include "xs3_math.h"
    #include "bfp_math.h"
    void vnr_inference_init(vnr_ie_state_t *ie_ptr);
    void vnr_inference(vnr_ie_state_t *ie_state, float_s32_t *vnr_output, bfp_s32_t *features);
#ifdef __cplusplus
}
#endif

#endif
