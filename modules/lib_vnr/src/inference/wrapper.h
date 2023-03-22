#pragma once

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
    int32_t vnr_init();
    int8_t* vnr_get_input();
    int8_t* vnr_get_output();
    void vnr_inference_invoke();
#ifdef __cplusplus
}
#endif

