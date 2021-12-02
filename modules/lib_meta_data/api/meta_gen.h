// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include <xs3_math_types.h>

/**
 * @brief Calculate the reference energy of the signal.
 * 
 * This API computes the dot product of each channel with itself to get the energy of the signal.
 * Then compares energy of each channel to return the maximum energy float_s32_t value.
 * The output can be assumed to be in 1.31 format.
 * 
 * @param[in] x                     Pointer array where each element points to the signal channel
 * @param[in] frame_length          Lenght of the signal frame
 * @param[in] num_chan              Number of channels 
 * 
 * @returns Reference energy
 * 
 */
C_API
float_s32_t get_max_ref_energy(
    int32_t * x[], 
    unsigned frame_length, 
    int num_chan);


/**
 * @brief Calculates a correlation factor between mic and AEC estimate of the mic signal.
 * 
 * Uses both inputs to compute AEC correlation factor.
 * 
 * @param[in] mic_data              Mic data array
 * @param[in] y_hat                 Estimated mic signal
 * 
 * @return Correlation factor of the filter 
 */
C_API
float_s32_t get_aec_corr_factor(
    const int32_t * mic_data, 
    const bfp_s32_t * y_hat);
