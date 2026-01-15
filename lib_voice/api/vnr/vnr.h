// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include "vnr_features_api.h"
#include "vnr_inference_api.h"

/**
 * @page page_vnr_h vnr.h
 *
 * This header contains lib_vnr API functions.
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_api   VNR API functions
 */

/**
 * @brief Initialise the VNR state
 *
 * This function should be called once at device startup.
 *
 * @param[inout] vnr pointer to the VNR state structure
 *
 * @ingroup vnr_api
 */
void vnr_state_init(vnr_state_t *vnr);

/**
 * @brief Calculate the Voice to Noise Ratio estimation from a frame of input data
 *
 * This function takes a frame of new samples, converts them to features and passes those to the inference engine.
 * The VNR output is a single value ranging between 0 and 1 returned in float_s32_t format, with 0 being the lowest SNR
 * and 1 being the strongest possible SNR in speech compared to noise.
 *
 * @param[inout] vnr     VNR state structure
 * @param[out] output    Pointer to return the resulting ratio
 * @param[in] input      Array of frame data on which to perform the VNR
 *
 * @ingroup vnr_api
 */
void vnr_process_frame(vnr_state_t * vnr, float_s32_t * output, int32_t input[VNR_FRAME_ADVANCE]);
