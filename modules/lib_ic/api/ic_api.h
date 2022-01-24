// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_API_H
#define IC_API_H

#include <stdio.h>
#include <string.h>
#include "xs3_math.h"
#include "bfp_math.h"
#include "ic_state.h"

/**
 * @page page_ic_api_h ic_api.h
 * 
 * lib_ic public functions API.
 *
 * @ingroup ic_header_file
 */

/**
 * @defgroup ic_func     High Level API Functions
 * @defgroup ic_low_level_func   Low Level API Functions
 */ 

/**
 * @brief Initialise IC data structures and set parameters according to ic_defines.h
 *
 * This is the first function that is called after creating and ic_state_t instance.
 *
 * @param[inout] state IC state structure. 
 *
 * @ingroup ic_func
 */void ic_init(ic_state_t *state);

/**
 * @brief Filter one frame of audio data inside the IC
 *
 * This should be called once per frame of IC_FRAME_ADVANCE samples
 *
 * @param[inout] state IC state structure. 
 * @param[in] y_data reference to mic 0 input buffer array
 * @param[in] x_data reference to mic 1 input buffer array
 * @param[out] output reference to IC output buffer array
 *
 * @ingroup ic_func
 */
void ic_filter(ic_state_t *state,
                      int32_t y_data[IC_FRAME_ADVANCE],
                      int32_t x_data[IC_FRAME_ADVANCE],
                      int32_t output[IC_FRAME_ADVANCE]);

/**
 * @brief Adapts the IC filter according to previous frame statistics and vad input
 *
 * This should be called after each call to ic_filter.
 * Filter and adapt are seprated so that the VAD function can operate on filtered samples.
 *
 * @param[inout] state IC state structure. 
 * @param[in] vad probability between 0 (0% VAD probability) and 255 (100% VAD probability)
 * @param[in] output pointer to previously filtered output samples.
 *
 * @ingroup ic_func
 */
void ic_adapt(ic_state_t *state,
                      uint8_t vad,
                      int32_t output[IC_FRAME_ADVANCE]);



#ifdef __XC__
#error PLEASE CALL IC FROM C TO AVOID STRUCT INCOMPATIBILITY ISSUES
#endif

#endif //IC_API_H