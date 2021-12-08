// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef SUP_STATE_H
#define SUP_STATE_H

#include <sup_noise_state.h>

typedef struct {

    bfp_s32_t S;
    bfp_s32_t S_min;
    bfp_s32_t S_tmp;
    bfp_s32_t p;
    bfp_s32_t alpha_d_title;
    bfp_s32_t lambda_hat;
    bfp_s32_t bin_gain;

    int32_t data_S [SUP_PROC_FRAME_BINS];
    int32_t data_S_min [SUP_PROC_FRAME_BINS];
    int32_t data_S_tmp [SUP_PROC_FRAME_BINS];
    int32_t data_p [SUP_PROC_FRAME_BINS];
    int32_t data_adt [SUP_PROC_FRAME_BINS];
    int32_t data_lambda_hat [SUP_PROC_FRAME_BINS];
    int32_t data_bin_gain [SUP_PROC_FRAME_BINS];

    bfp_s32_t prev_frame;
    bfp_s32_t overlap;

    int32_t data_prev_frame [SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE];
    int32_t data_ovelap [SUP_FRAME_ADVANCE];

    float_s32_t delta;
    float_s32_t noise_floor;
    float_s32_t alpha_gain;
    float_s32_t alpha_d;
    float_s32_t alpha_s;
    float_s32_t alpha_p;

    unsigned reset_period;
    unsigned reset_counter;

} suppression_state_t;

/*
 * Y is a complex array of length length-1
 * new_mag is a complex array of length length
 * original_mag is a complex array of length length
 */
void sup_rescale_vector(bfp_complex_s32_t * Y, 
                        bfp_s32_t * new_mag,
                        bfp_s32_t * orig_mag);

//void sup_rescale(dsp_complex_t &Y, uint32_t new_mag, uint32_t original_mag);

/*
 * window is an array of q1_31 of length (window_length/2) it is applied symmetrically to the frame
 * from sample 0 (oldest) to (window_length/2 - 1) then zeros are applied to the end of the frame.
 */
void sup_apply_window(bfp_s32_t * input, 
                    const int32_t * window, 
                    const int32_t * rev_window, 
                    const unsigned frame_length, 
                    const unsigned window_length);

void sup_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap);

void ns_process_frame(bfp_s32_t * abs_Y, suppression_state_t * state);


//#include "suppression_control.h"

#endif