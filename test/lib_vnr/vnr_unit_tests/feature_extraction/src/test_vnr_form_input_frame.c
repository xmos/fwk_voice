// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr_features_api.h"

static vnr_input_state_t vnr_input_state;
void test_init()
{
    vnr_input_state_init(&vnr_input_state);
}

void test(int32_t *output, int32_t *input)
{
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr_input_state, &X, input_frame, input);
    //printf("X.exp = %d, X.length = %d\n",X.exp, X.length); 
    memcpy(output, &X.exp, sizeof(int32_t));
    memcpy(&output[1], X.data, X.length*sizeof(complex_s32_t));
}
