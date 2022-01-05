// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_API_H
#define IC_API_H

#include <stdio.h>
#include <string.h>
#include "xs3_math.h"
#include "bfp_math.h"
#include "ic_state.h"

//IC public API
void ic_init(ic_state_t *state);

//Some nice doxygen comments
void ic_filter(ic_state_t *state,
                      int32_t y_data[IC_FRAME_ADVANCE],
                      int32_t x_data[IC_FRAME_ADVANCE],
                      int32_t output[IC_FRAME_ADVANCE]);

//Some nice doxygen comments
void ic_adapt(ic_state_t *state,
                      uint8_t vad,
                      int32_t output[IC_FRAME_ADVANCE]);


#endif //IC_API_H