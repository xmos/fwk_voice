#pragma once
#include <stdint.h>
#include "xmath/xmath.h"
/** Quantisation spec used to quantise the VNR input features and dequantise the VNR output according to the specification for TensorFlow Lite's 8-bit quantization scheme
 * Quantisation: q = f/input_scale + input_zero_point
 * Dequantisation: f = output_scale * (q + output_zero_point)
 */
typedef struct {
    float_s32_t input_scale_inv; // Inverse of the input scale
    float_s32_t input_zero_point;
    float_s32_t output_scale;
    float_s32_t output_zero_point;
}vnr_model_quant_spec_t;

#ifdef __cplusplus
extern "C" {
#endif
    int32_t vnr_init(vnr_model_quant_spec_t * quant_spec);
    int8_t* vnr_get_input();
    int8_t* vnr_get_output();
    void vnr_inference_invoke();
#ifdef __cplusplus
}
#endif

