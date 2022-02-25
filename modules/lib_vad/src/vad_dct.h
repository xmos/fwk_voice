// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef VAD_DCT_H_
#define VAD_DCT_H_

#include "stdint.h"

/** This function performs a 24 point DCT
 *
 *  The first output is the DC value, subsequent values are the values for
 *  the basis vectors of half a cosine, a whole cosine, 1.5 cosine, 2
 *  consines, etc.
 *
 *  \param  input           input values to the DCT
 *  \param  output          DCT values.
 */
void vad_dct_forward24(int32_t output[24], int32_t input[24]);

#endif