#pragma once
#include "vnr_inference_priv.h"
#include <stdint.h>
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

