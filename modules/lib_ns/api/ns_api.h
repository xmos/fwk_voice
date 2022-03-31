// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef NS_API_H
#define NS_API_H

#include "ns_state.h"

/**
 * @page page_ns_api_h ns_api.h
 * 
 * This header should be included in application source code to gain access to the
 * lib_ns public functions API.
 */

/**
 * @defgroup ns_func NS API functions 
 */

/** 
 * @brief Initialise the NS
 *
 * This function initialises the NS state with the provided configuration. It must be called
 * at startup to initialise the NS before processing any frames, and can be called at any time
 * after that to reset the NS instance, returning the internal NS state to its defaults.
 *
 * @param[out] ns    NS state structure
 * 
 * @par Example
 * @code{.c}
 *      ns_state_t ns;
 *      ns_init(&ns);
 * @endcode
 * 
 * @ingroup ns_func
 */
void ns_init(ns_state_t * ns);


/**
 * @brief Perform NS processing on a frame of input data
 * 
 * This function updates the NS's internal state based on the input 1.31 frame, and
 * returns an output 1.31 frame containing the result of the NS algorithm applied to the input.
 *
 * The `input` and `output` pointers can be equal to perform the processing in-place.
 * 
 * @param[inout] ns     NS state structure
 * @param[out] output    Array to return the resulting frame of data
 * @param[in] input      Array of frame data on which to perform the NS
 * 
 * @par Example
 * @code{.c}
 *      int32_t input[NS_FRAME_ADVANCE];
 *      int32_t output[NS_FRAME_ADVANCE];
 *      ns_state_t ns;
 *      ns_init(&ns);
 *      ns_process_frame(&ns, output, input);
 * @endcode
 * 
 * @ingroup ns_func
 */
void ns_process_frame(ns_state_t * ns,
                        int32_t output [NS_FRAME_ADVANCE],
                        const int32_t input[NS_FRAME_ADVANCE]);


#endif
