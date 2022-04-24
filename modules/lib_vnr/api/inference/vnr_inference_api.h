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
    void vnr_priv_output_dequantise(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_ie_config_t *ie_config);
    void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_ie_config_t *ie_config);
#ifdef __cplusplus
}
#endif

#endif
