// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <suppression.h>
#include <bfp_s32.h>

#ifndef SUP_NOISE_STATE_H
#define SUP_NOISE_STATE_H

typedef struct { ///////////////////ORIGINAL/////////////////////
    uint32_t S[SUP_PROC_FRAME_BINS];
    uint32_t S_min[SUP_PROC_FRAME_BINS];
    uint32_t S_tmp[SUP_PROC_FRAME_BINS];
    uint32_t p[SUP_PROC_FRAME_BINS];
    uint32_t alpha_d_tilde[SUP_PROC_FRAME_BINS];  //Q32
    uint32_t lambda_hat[SUP_PROC_FRAME_BINS];

    int32_t  prev_frame[SUP_FRAME_ADVANCE];

    uint32_t bin_gain[SUP_PROC_FRAME_BINS];

    unsigned reset_counter;
    int S_exp; //this is for all the S arrays
    unsigned S_headroom;
    int lambda_hat_exp;
    unsigned lambda_hat_headroom;

    uint32_t reset_period;
    uint32_t alpha_d;
    uint32_t alpha_s;
    uint32_t alpha_p;
    int32_t delta;
    uint32_t noise_floor; //linear - not db

    uint32_t alpha_gain;  //This controls the s-curve of bin gain
    //For each output that is above the noise floor (given by noise_floor) the
    //gain will move towards 1.0. For each output that would be beloe the noise
    //floor the gain moves towards 0.0. alpha_gain controls the speed at which
    //this happens. 1.0 = fast acting, 0.0 = slow acting. 1.0 = disabled.

} ns_state_t1;

typedef struct {///////////////////////LIB_SX3_MATH ADAPTED///////////////////////////

    bfp_s32_t S, S_min, S_tmp, lambda_hat;
    bfp_s32_t p, alpha_d_tilde, prev_frame, bin_gain;

    unsigned reset_counter;
    
    int32_t reset_period;
    int32_t alpha_d;
    int32_t alpha_s;
    int32_t alpha_p;
    int32_t delta;
    int32_t noise_floor; //linear - not db

    int32_t alpha_gain;  //This controls the s-curve of bin gain
    //For each output that is above the noise floor (given by noise_floor) the
    //gain will move towards 1.0. For each output that would be beloe the noise
    //floor the gain moves towards 0.0. alpha_gain controls the speed at which
    //this happens. 1.0 = fast acting, 0.0 = slow acting. 1.0 = disabled.

} ns_state_t;


void ns_dump_parameters(ns_state_t * state);

//For testing
/*
void ns_update_S(uint32_t * abs_Y, unsigned length, int Y_exp,
        ns_state_t &state);

void ns_minimum(uint32_t * alias A, uint32_t * alias B, uint32_t * alias C,
        unsigned length, int B_exp, int C_exp);

void ns_minimum_asm(uint32_t * alias A, uint32_t * alias B, uint32_t * alias C,
        unsigned length, int B_exp, int C_exp);

void ns_update_p(ns_state_t &state, unsigned length);

void ns_update_alpha_d_tilde(uint32_t p[], uint32_t alpha_d_tilde[],
                             uint32_t alpha_d, unsigned length);

void ns_update_alpha_d_tilde_asm(uint32_t p[], uint32_t alpha_d_tilde[],
                                 uint32_t alpha_d, unsigned length);

void ns_update_lambda_hat(uint32_t * abs_Y, unsigned length, int Y_exp,
        ns_state_t &state);

int ns_update_and_test_reset(ns_state_t &state);

void ns_sqrt(uint32_t &s, int &v_exp, const unsigned hr);


void ns_subtract_lambda_from_frame(uint32_t * abs_Y, unsigned length, int &Y_exp,
                                  ns_state_t &state);

void ns_increase_gain(uint32_t &bin_gain, uint32_t alpha_gain);
void ns_decrease_gain(uint32_t &bin_gain, uint32_t alpha_gain);
*/
#endif
