#ifndef __VNR_INFERENCE_PRIV_H__
#define __VNR_INFERENCE_PRIV_H__

#include "xmath/xmath.h"
#include "wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
    /**
     * @brief Quantise VNR features
     * This function quantises the floating point features according to the specification for TensorFlow Lite's 8-bit quantization scheme.
     * @param[out] quantised_patch quantised feature patch
     * @param[in] normalised_patch feature patch before quantisation. Note that this is not passed as a const pointer and the normalised_patch memory is overwritten
     * during quantisation calculation.
     * @param[in] quant_spec TensorFlow Lite's 8-bit quantisation specification
     */
    void vnr_priv_feature_quantise(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_model_quant_spec_t *quant_spec);

    /**
     * @brief Deuquantise Inference output
     * This function dequantises the VNR model output according to the specification for TensorFlow Lite's 8-bit quantization scheme.
     * @param[out] dequant_output Dequantised output
     * @param[in] quant_output VNR model inference quantised output
     * @param[in] quant_spec TensorFlow Lite's 8-bit quantisation specification
     */
    void vnr_priv_output_dequantise(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_model_quant_spec_t *quant_spec);
#ifdef __cplusplus
}
#endif

#endif
