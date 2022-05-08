// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr_features_api.h"
#include "vnr_features_priv.h"

void test_init()
{
}

void test(int32_t *output, int32_t *input) {
    complex_s32_t DWORD_ALIGNED data[VNR_FD_FRAME_LENGTH];
    memcpy(data, &input[1], VNR_FD_FRAME_LENGTH*sizeof(complex_s32_t)); // input[0] is the exponent
    
    bfp_complex_s32_t X;
    bfp_complex_s32_init(&X, data, input[0], VNR_FD_FRAME_LENGTH, 1);
    //float_s32_t mel_output[VNR_MEL_FILTERS];

    vnr_priv_mel_compute((float_s32_t*)output, &X);

}
