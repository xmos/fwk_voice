// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#pragma once

#include <xs3_math_types.h>

/**
 * @brief Calculate the reference energy of the signal.
 * 
 * This API computes the dot product of each channel with itself to get the energy of the signal.
 * Then compares energy of each channels to return the maximum energy float_s32_t value.
 * 
 * @param[in] x                     Pointer array where each element points to the signal channel
 * @param[in] frame_length          Lenght of the signal frame
 * @param[in] NUM_CHAN              Number of channels 
 * 
 * @returns Reference energy
 * 
 */
C_API
float_s32_t get_max_ref_energy(
    int32_t * x[], 
    unsigned frame_length, 
    int NUM_CHAN);


/**
 * @brief Calculate the AEC correlation factor.
 * 
 * Takes a part of both arrays to compute AEC correlation factor.
 * 
 * @param[in] mic_data              Mic data array
 * @param[in] y_hat                 Filter output
 * 
 * @return Correlation factor of the filter 
 */
C_API
float_s32_t get_aec_corr_factor(
    const int32_t * mic_data, 
    const bfp_s32_t * y_hat);
