// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr_features_api.h"
#include "vnr_features_priv.h"

static vnr_input_state_t vnr_input_state;
void test_init()
{
    vnr_input_state_init(&vnr_input_state);
}

void test(int32_t *output, int32_t *input)
{
    int32_t enable_highpass = input[VNR_FRAME_ADVANCE]; // Last value in the input is the hpf enable flag
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr_input_state, &X, input_frame, input);

    vnr_priv_make_slice(output, &X, enable_highpass);
}
