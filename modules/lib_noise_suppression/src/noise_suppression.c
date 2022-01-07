// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <xs3_math.h>
#include <bfp_init.h>
#include <bfp_s32.h>
#include <stdio.h>

#include <suppression_state.h>
#include "suppression_ns.h"
#include "suppression_testing.h"

#define one_mant 1073741824
#define one_exp -30

void ns_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main){

    bfp_s32_use_exponent(A, main->exp);
    bfp_s32_use_exponent(B, main->exp);
    int min_hr = (A->hr <= B->hr) ? A->hr : B->hr;
    min_hr = (min_hr <= main->hr) ? min_hr : main->hr;
    A->hr = min_hr;
    B->hr = min_hr;
}

// A = min (B, C) - element wise
// A_exp = C_exp
void ns_minimum(bfp_s32_t * dst, bfp_s32_t * src1, bfp_s32_t * src2){

    for (int v = 0; v < dst->length; v++){
        dst->data[v] = (src1->data[v] <= src2->data[v]) ? src1->data[v] : src2->data[v];
    }
    bfp_s32_headroom(dst);
}

//S = alpha_s*S + (1.0-alpha_s)*(abs(Y)**2)
void ns_update_S(bfp_s32_t * abs_Y,
        sup_state_t *state){
    int32_t scratch[SUP_PROC_FRAME_BINS];
    bfp_s32_t tmp;
    float_s32_t t;
    bfp_s32_init(&tmp, scratch, INT_MAX, state->S.length, 0);

    bfp_s32_mul(&tmp, abs_Y, abs_Y);

    t.mant = one_mant;
    t.exp = one_exp; // t = 1

    bfp_s32_scale(&tmp, &tmp, float_s32_sub(t, state->alpha_s));
    
    bfp_s32_scale(&state->S, &state->S, state->alpha_s);

    bfp_s32_add(&state->S, &state->S, &tmp);
}

//    S_r = S/S_min
//    I = S_r > delta
//    p = alpha_p * p + (1.0 - alpha_p) * I
void ns_update_p(sup_state_t * state){

    bfp_s32_t tmp;
    float_s32_t t;
    int32_t scratch [SUP_PROC_FRAME_BINS];
    int32_t one_zero [SUP_PROC_FRAME_BINS];
    bfp_s32_init(&tmp, scratch, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_inverse(&tmp, &state->S_min);
    bfp_s32_mul(&tmp, &tmp, &state->S);

    for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
        t.mant = tmp.data[v];
        t.exp = tmp.exp;
        one_zero[v] = INT_MAX * float_s32_gt(t, state->delta);
    }

    bfp_s32_init(&tmp, one_zero, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 1);

    t.mant = one_mant;
    t.exp = one_exp; // t = 1

    t = float_s32_sub(t, state->alpha_p);

    bfp_s32_scale(&tmp, &tmp, t);

    bfp_s32_scale(&state->p, &state->p, state->alpha_p);
    bfp_s32_add(&state->p, &state->p, &tmp);
}

//    alpha_d_tilde = alpha_d + (1.0 - alpha_d) * p
void ns_update_alpha_d_tilde(sup_state_t * state){
    bfp_s32_t tmp;
    float_s32_t t;
    int32_t scratch [SUP_PROC_FRAME_BINS];

    bfp_s32_init(&tmp, scratch, state->p.exp, state->p.length, 0);

    t.mant = one_mant;
    t.exp = one_exp; //t = 1

    t = float_s32_sub(t, state->alpha_d);

    bfp_s32_scale(&tmp, &state->p, t);

    bfp_s32_add_scalar(&state->alpha_d_tilde, &tmp, state->alpha_d);
}

//    lambda_hat = alpha_d_tilde[i]*lambda_hat + (1.0 - alpha_d_tilde)*np.square(np.absolute(Y))
void ns_update_lambda_hat(bfp_s32_t * abs_Y, sup_state_t * state){
    bfp_s32_t tmp1, tmp2;
    float_s32_t one, t;
    int32_t scratch1 [SUP_PROC_FRAME_BINS];
    int32_t scratch2 [SUP_PROC_FRAME_BINS];
    bfp_s32_init(&tmp1, scratch1, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&tmp2, scratch2, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_mul(&tmp1, abs_Y, abs_Y);

    t.mant = -one_mant;
    t.exp = one_exp; // t = -1

    bfp_s32_scale(&tmp2, &state->alpha_d_tilde, t);

    t.mant = one_mant; //t = 1

    bfp_s32_add_scalar(&tmp2, &tmp2, t);
    bfp_s32_mul(&tmp1, &tmp1, &tmp2);
    bfp_s32_mul(&state->lambda_hat, &state->lambda_hat, &state->alpha_d_tilde);
    bfp_s32_add(&state->lambda_hat, &state->lambda_hat, &tmp1);
}

int ns_update_and_test_reset(sup_state_t * state){
    unsigned count = state->reset_counter;
    count += SUP_FRAME_ADVANCE;
    if(count > state->reset_period){
        state->reset_counter = count - state->reset_period;
        return 1;
    } else {
        state->reset_counter = count;
        return 0;
    }
}

#define LUT_SIZE 64
#define LUT_INPUT_MULTIPLIER 4
const int32_t LUT[LUT_SIZE] = {
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2936779382,  2936779382,  2936779382,  2936779382,  
2278108010,  1767165807,  1370819547,  1063367242,  
824871438,  639866325,  496354819,  385030587,  
298674552,  231686756,  179723223,  139414256,  
108145929,  83890573,  65075296,  50479976,  
39158146,  30375617,  23562865,  18278102,  
14178624,  10998591,  8531788,  6618248,  
5133883,  3982438,  3089242,  2396376,  
1858908,  1441985,  1118572,  867694,  
673085,  522123,  405019,  314180,  
243714,  189053,  146651,  113760,  
88245,  68453,  53100,  41191,  
31952,  24786,  19227,  14914,  
};

void ns_subtract_lambda_from_frame(bfp_s32_t * abs_Y, sup_state_t * state){
    bfp_s32_t sqrt_lambda, tmp, comp;
    int32_t scratch1 [SUP_PROC_FRAME_BINS];
    int32_t scratch2 [SUP_PROC_FRAME_BINS];
    float_s32_t t;
    int32_t t_data;
    bfp_s32_init(&sqrt_lambda, scratch1, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&tmp, scratch2, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_sqrt(&sqrt_lambda, &state->lambda_hat);

    t.mant = one_mant;
    t.exp = one_exp - 2; // t = 0.25

    bfp_s32_scale(&sqrt_lambda, &sqrt_lambda, t);

    bfp_s32_inverse(&tmp, &sqrt_lambda);
    bfp_s32_mul(&tmp, &tmp, abs_Y);

    t.mant = one_mant;
    t.exp = one_exp + 2; // t = 4

    bfp_s32_scale(&sqrt_lambda, &sqrt_lambda, t);

    t = bfp_s32_min(&tmp);
    t_data = t.mant;
    bfp_s32_init(&comp, &t_data, t.exp, 1, 1);
    bfp_s32_use_exponent(&comp, 0);

    int32_t index = (comp.data[0] < (LUT_SIZE - 1)) ? comp.data[0] : (LUT_SIZE - 1);
    t.mant = LUT[index];
    t.exp = SUP_INT_EXP;

    bfp_s32_scale(&tmp, &sqrt_lambda, t);
    bfp_s32_sub(abs_Y, abs_Y, &tmp);
    bfp_s32_rect(abs_Y, abs_Y);
}

/*
 * abs_Y is the current estimated mag of Y, i.e. it may have been echo suppressed or dereverbed.
 * Y_exp is the exponent of abs_Y
 *
 */
void ns_process_frame(bfp_s32_t * abs_Y, sup_state_t * state){

    ns_update_S(abs_Y, state);

    ns_adjust_exp(&state->S_min, &state->S_tmp, &state->S);

    if(ns_update_and_test_reset(state)){
        ns_minimum(&state->S_min, &state->S_tmp, &state->S);
        memcpy(&state->S_tmp, &state->S, sizeof(state->S));
    } else {
        ns_minimum(&state->S_min, &state->S_min, &state->S);
        ns_minimum(&state->S_tmp, &state->S_tmp, &state->S);
    }

    ns_update_p(state);

    ns_update_alpha_d_tilde(state);

    ns_update_lambda_hat(abs_Y, state);

    ns_subtract_lambda_from_frame(abs_Y, state);

}