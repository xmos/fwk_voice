// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_XC_TESTAPP_H
#define AEC_XC_TESTAPP_H

#ifdef __XC__
extern "C" {
    #include "aec_defines.h"
}
#else
    #include "aec_defines.h"
#endif
#include "aec_memory_pool.h"

void aec_testapp_process_frame(
        aec_state_t *main_state,
        aec_state_t *shadow_state,
        const int32_t (*y_data)[AEC_FRAME_ADVANCE],
        const int32_t (*x_data)[AEC_FRAME_ADVANCE],
        int32_t (*output_main)[AEC_FRAME_ADVANCE],
        int32_t (*output_shadow)[AEC_FRAME_ADVANCE]);
#endif
