// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "vnr_features_api.h"

void vnr_input_state_init(vnr_input_state_t *input_state) {
    memset(input_state, 0, sizeof(vnr_input_state_t));
}

void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, const int32_t *new_x_frame) {
    // Update current frame
    memcpy(x_data, input_state->prev_input_samples, (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
    memcpy(&x_data[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE], new_x_frame, VNR_FRAME_ADVANCE*sizeof(int32_t));

    // Update previous samples
    memcpy(input_state->prev_input_samples, &x_data[VNR_FRAME_ADVANCE], (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
}

void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data) {
    bfp_s32_t x;
    bfp_s32_init(&x, x_data, VNR_INPUT_EXP, VNR_PROC_FRAME_LENGTH, 1);

    bfp_complex_s32_t *temp = bfp_fft_forward_mono(&x);
    bfp_fft_unpack_mono(temp);
    memcpy(X, temp, sizeof(bfp_complex_s32_t));
}

static int framenum=0;
void vnr_extract_features(vnr_input_state_t *input_state, const int32_t *new_x_frame, /*for debug*/ bfp_complex_s32_t *fft_output)
{
    int32_t DWORD_ALIGNED x_data[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
    vnr_form_input_frame(input_state, x_data, new_x_frame);
    
    bfp_complex_s32_t X;
    vnr_forward_fft(&X, x_data);

    //For debug.
    fft_output->hr = X.hr;
    fft_output->exp = X.exp;
    fft_output->length = X.length;
    memcpy(fft_output->data, X.data, fft_output->length*sizeof(complex_s32_t));

    // Normalise to log max precision output
    left_shift_t ls = bfp_complex_s32_headroom(fft_output); // TODO Why is this not the same as fft_output->hr?
    bfp_complex_s32_shl(fft_output, fft_output, ls);
    fft_output->exp -= ls;

    framenum++;
}
