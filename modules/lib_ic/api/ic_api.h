// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_API_H
#define IC_API_H

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
 * @brief Initialise IC and VNR data structures and set parameters according to ic_defines.h
 *
 * This is the first function that must called after creating an ic_state_t instance.
 *
 * @param[inout] state pointer to IC state structure
 * @returns Error status of the VNR inference engine initialisation that is done as part of ic_init. 0 if no error, one of TfLiteStatus error enum values in case of error. 
 * @ingroup ic_func
 */
int32_t ic_init(ic_state_t *state);


/**
 * @brief Filter one frame of audio data inside the IC
 *
 * This should be called once per new frame of IC_FRAME_ADVANCE samples.
 * The y_data array contains the microphone data that is to have the
 * noise subtracted from it and x_data is the noise reference source which
 * is internally delayed before being fed into the adaptive filter. 
 * Note that the y_data input array is internally delayed by the call to 
 * ic_filter() and so contains the delayed y_data afterwards.
 * Typically it does not matter which mic channel is connected to x or y_data
 * as long as the separation is appropriate. The performance of this filter
 * has been optimised for a 71mm mic separation distance. 
 *
 * @param[inout] state pointer to IC state structure
 * @param[inout] y_data array reference of mic 0 input buffer. Modified during call
 * @param[in] x_data array reference of mic 1 input buffer
 * @param[out] output array reference containing IC processed output buffer
 *
 * @ingroup ic_func
 */
void ic_filter(ic_state_t *state,
                      int32_t y_data[IC_FRAME_ADVANCE],
                      int32_t x_data[IC_FRAME_ADVANCE],
                      int32_t output[IC_FRAME_ADVANCE]);
/**
 * @brief Calculate voice to noise ratio estimation for the input and output of the IC
 *
 * This function can be called after each call to ic_filter.
 * It will calculate voice to noise ratio which can be used to give information
 * to ic_adapt and to the AGC.
 *
 * @param[inout] state pointer to IC state structure
 * @param[inout] input_vnr_pred voice to noise estimate of the IC input
 * @param[inout] output_vnr_pred voice to noise estimate of the IC output
 *
 * @ingroup ic_func
 */
void ic_calc_vnr_pred(ic_state_t *state,
		      float_s32_t * input_vnr_pred,
		      float_s32_t * output_vnr_pred);

/**
 * @brief Adapts the IC filter according to previous frame's statistics and VNR input
 *
 * This function should be called after each call to ic_filter.
 * Filter and adapt functions are separated so that the external VNR can operate
 * on each frame.
 *
 * @param[inout] state pointer to IC state structure
 * @param[in] vnr VNR Voice-to-Noise ratio estimation
 *
 * @ingroup ic_func
 */
void ic_adapt(ic_state_t *state,
                      float_s32_t vnr);

#ifdef __XC__
#error PLEASE CALL IC FROM C TO AVOID STRUCT INCOMPATIBILITY ISSUES
#endif

#endif //IC_API_H
