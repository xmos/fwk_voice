#ifndef __VNR_INFERENCE_STATE_H__
#define __VNR_INFERENCE_STATE_H__
#include "xs3_math.h"

typedef struct {
    float_s32_t input_scale_inv; // 1/input_scale
    float_s32_t input_zero_point;
    float_s32_t output_scale;
    float_s32_t output_zero_point;
}vnr_ie_config_t;

#endif
