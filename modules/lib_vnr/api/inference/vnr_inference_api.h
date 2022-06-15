#ifndef __VNR_INFERENCE_API_H__
#define __VNR_INFERENCE_API_H__

#ifdef __cplusplus
extern "C" {
#endif
    #include "bfp_math.h"
    int32_t vnr_inference_init();
    void vnr_inference(float_s32_t *vnr_output, bfp_s32_t *features);
#ifdef __cplusplus
}
#endif

#endif
