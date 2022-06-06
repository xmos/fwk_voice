#ifndef __VNR_INFERENCE_STATE_H__
#define __VNR_INFERENCE_STATE_H__

#include "vnr_model_data.h"
#include "xtflm_conf.h"
#include "vnr_tensor_arena_size.h"

#include "xs3_math.h"

typedef struct {
    float_s32_t input_scale_inv; // 1/input_scale
    float_s32_t input_zero_point;
    float_s32_t output_scale;
    float_s32_t output_zero_point;
}vnr_ie_config_t;

typedef struct {
    uint64_t tensor_arena[(TENSOR_ARENA_SIZE_BYTES + sizeof(uint64_t) - 1)/sizeof(uint64_t)];
    int8_t *input_buffer;
    size_t input_size;
    int8_t *output_buffer;
    size_t output_size;
    vnr_ie_config_t ie_config;
}vnr_ie_state_t;

#endif
