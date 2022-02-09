// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef SUP_STATE_H
#define SUP_STATE_H

#include <xs3_math_types.h>

#define SUP_FRAME_ADVANCE           (240)
#define SUP_PROC_FRAME_LENGTH      (512) 
#define SUP_PROC_FRAME_BINS        ((SUP_PROC_FRAME_LENGTH / 2) + 1)
#define SUP_INT_EXP (-31)
#define SUPPRESSION_WINDOW_LENGTH (480)

typedef struct {

    //Dynamic MCRA filter coefficients
    bfp_s32_t S;
    bfp_s32_t S_min;
    bfp_s32_t S_tmp;
    bfp_s32_t p;
    bfp_s32_t alpha_d_tilde;
    bfp_s32_t lambda_hat;

    int32_t data_S [SUP_PROC_FRAME_BINS];
    int32_t data_S_min [SUP_PROC_FRAME_BINS];
    int32_t data_S_tmp [SUP_PROC_FRAME_BINS];
    int32_t data_p [SUP_PROC_FRAME_BINS];
    int32_t data_adt [SUP_PROC_FRAME_BINS];
    int32_t data_lambda_hat [SUP_PROC_FRAME_BINS];

    //Data needed for the frame packing and windowing
    bfp_s32_t prev_frame;
    bfp_s32_t overlap;
    bfp_s32_t wind;
    bfp_s32_t rev_wind;

    int32_t data_prev_frame [SUP_PROC_FRAME_LENGTH - SUP_FRAME_ADVANCE];
    int32_t data_ovelap [SUP_FRAME_ADVANCE];
    int32_t data_rev_wind [SUPPRESSION_WINDOW_LENGTH / 2];

    //Static MCRA filter coefficients
    float_s32_t delta;
    float_s32_t alpha_d;
    float_s32_t alpha_s;
    float_s32_t alpha_p;
    float_s32_t one_minus_aplha_d;

    unsigned reset_period;
    unsigned reset_counter;

} sup_state_t;

#endif