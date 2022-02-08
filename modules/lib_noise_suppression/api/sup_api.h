// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef SUP_API_H
#define SUP_API_H

#include "sup_state.h"

/**
 * @page page_sup_api_h sup_api.h
 * 
 * This header should be included in application source code to gain access to the
 * lib_noise_suppression public functions API.
 */

/**
 * @defgroup sup_func NS API functions 
 */

/** 
 * @brief Initialise the NS
 *
 * This function initialises the NS state with the provided configuration. It must be called
 * at startup to initialise the NS before processing any frames, and can be called at any time
 * after that to reset the NS instance, returning the internal NS state to its defaults.
 *
 * @param[out] sup    NS state structure
 * 
 * @par Example
 * @code{.c}
 *      sup_state_t sup;
 *      sup_init(&sup);
 * @endcode
 * 
 * @ingroup sup_func
 */
void sup_init(sup_state_t * sup);


/**
 * @brief Perform NS processing on a frame of input data
 * 
 * This function updates the NS's internal state based on the input 1.31 frame, and
 * returns an output 1.31 frame containing the result of the NS algorithm applied to the input.
 *
 * The `input` and `output` pointers can be equal to perform the processing in-place.
 * 
 * @param[inout] sup     NS state structure
 * @param[out] output    Array to return the resulting frame of data
 * @param[in] input      Array of frame data on which to perform the NS
 * 
 * @par Example
 * @code{.c}
 *      int32_t input[SUP_FRAME_ADVANCE];
 *      int32_t output[SUP_FRAME_ADVANCE];
 *      sup_state_t sup;
 *      sup_init(&sup);
 *      sup_process_frame(&sup, output, input);
 * @endcode
 * 
 * @ingroup sup_func
 */
void sup_process_frame(sup_state_t * sup,
                        int32_t output [SUP_FRAME_ADVANCE],
                        const int32_t input[SUP_FRAME_ADVANCE]);


#endif
