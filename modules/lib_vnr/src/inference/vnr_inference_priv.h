#ifndef __VNR_INFERENCE_PRIV_H__
#define __VNR_INFERENCE_PRIV_H__

#include "vnr_tensor_arena_size.h"
#include "bfp_math.h"

typedef struct {
    float_s32_t input_scale_inv; // 1/input_scale
    float_s32_t input_zero_point;
    float_s32_t output_scale;
    float_s32_t output_zero_point;
}vnr_model_quant_spec_t;

typedef struct {
    uint64_t tensor_arena[(TENSOR_ARENA_SIZE_BYTES + sizeof(uint64_t) - 1)/sizeof(uint64_t)];
    vnr_model_quant_spec_t quant_spec;
}vnr_ie_state_t;

#ifdef __cplusplus
extern "C" {
#endif
    void vnr_priv_init_quant_spec(vnr_model_quant_spec_t *quant_spec);
    void vnr_priv_output_dequantise(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_model_quant_spec_t *quant_spec);
    void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_model_quant_spec_t *quant_spec);
#ifdef __cplusplus
}
#endif

#endif
