// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <xs3_math.h>
#include <bfp_init.h>
#include <bfp_s32.h>

//#include <suppression.h>
#include <suppression_state.h>
//#include <suppression_conf.h>
#include <suppression_ns.h>
#include <suppression_testing.h>


#if SUP_DEBUG | SUP_DEBUG_PRINT | SUP_WARNING_PRINT
#include <stdio.h>
#endif

void ns_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main){

    bfp_s32_use_exponent(A, main->exp);
    bfp_s32_use_exponent(B, main->exp);
    int min_hr = (A->hr < B->hr) ? A->hr : B->hr;
    min_hr = (min_hr < main->hr) ? min_hr : main->hr;
    A->hr = min_hr;
    B->hr = min_hr;
}

// A = min (B, C) - element wise
// A_exp = C_exp
void ns_minimum(bfp_s32_t * dst, bfp_s32_t * src1, bfp_s32_t * src2){

    for (int v = 0; v < dst->length; v++){
        dst->data[v] = (src1->data[v] < src2->data[v]) ? src1->data[v] : src2->data[v];
    }
}

//S = alpha_s*S + (1.0-alpha_s)*(abs(Y)**2)
void ns_update_S(bfp_s32_t * abs_Y,
        suppression_state_t *state){
    int32_t scratch[SUP_PROC_FRAME_BINS];
    bfp_s32_t tmp;
    bfp_s32_init(&tmp, scratch, INT_MAX, state->S.length, 0);

    bfp_s32_mul(&tmp, abs_Y, abs_Y);

    bfp_s32_scale(&tmp, &tmp, float_s32_sub(float_to_float_s32(1.0), state->alpha_s));
    
    bfp_s32_scale(&state->S, &state->S, state->alpha_s);

    bfp_s32_add(&state->S, &state->S, &tmp);
}

//    S_r = S/S_min
//    I = S_r > delta
//    p = alpha_p * p + (1.0 - alpha_p) * I
void ns_update_p(suppression_state_t * state){

    bfp_s32_t tmp;
    int32_t scratch [SUP_PROC_FRAME_BINS] = {0};
    bfp_s32_init(&tmp, scratch, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_inverse(&tmp, &state->S_min);
    bfp_s32_mul(&tmp, &tmp, &state->S);

    for(int v = 0; v > SUP_PROC_FRAME_BINS; v++){
        float_s32_t t;
        t.mant = tmp.data[v]<<tmp.hr;
        t.exp = tmp.exp;
        tmp.data[v] = (float_s32_gt(t, state->delta)); ////////////////////////////TODO: review
    }
    tmp.exp = INT_EXP;
    tmp.hr = 0;
    bfp_s32_scale(&tmp, &tmp, float_s32_sub(float_to_float_s32(1.0), state->alpha_p));

    bfp_s32_scale(&state->p, &state->p, state->alpha_p);
    bfp_s32_add(&state->p, &state->p, &tmp);
}

//    alpha_d_tilde = alpha_d + (1.0 - alpha_d) * p
void ns_update_alpha_d_tilde(suppression_state_t * state){
    bfp_s32_t tmp;
    int32_t scratch [SUP_PROC_FRAME_BINS] = {0};
    bfp_s32_init(&tmp, scratch, INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_scale(&tmp, &state->p, float_s32_sub(float_to_float_s32(1.0), state->alpha_d));
    bfp_s32_add_scalar(&state->alpha_d_tilde, &tmp, state->alpha_d);
}

//    lambda_hat = alpha_d_tilde[i]*lambda_hat + (1.0 - alpha_d_tilde)*np.square(np.absolute(Y))
void ns_update_lambda_hat(bfp_s32_t * abs_Y, suppression_state_t * state){
    bfp_s32_t tmp1, tmp2;
    int32_t scratch1 [SUP_PROC_FRAME_BINS] = {0};
    int32_t scratch2 [SUP_PROC_FRAME_BINS] = {0};
    bfp_s32_init(&tmp1, scratch1, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&tmp2, scratch2, INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_mul(&tmp1, abs_Y, abs_Y);
    bfp_s32_add_scalar(&tmp2, &state->alpha_d_tilde, float_to_float_s32(-1.0));
    bfp_s32_mul(&tmp1, &tmp1, &tmp2);
    bfp_s32_mul(&state->lambda_hat, &state->lambda_hat, &state->alpha_d_tilde);
    bfp_s32_add(&state->lambda_hat, &state->lambda_hat, &tmp1);
}

int ns_update_and_test_reset(suppression_state_t * state){
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

void ns_subtract_lambda_from_frame(bfp_s32_t * abs_Y, suppression_state_t * state){
    bfp_s32_t sqrt_lambda, tmp, comp;
    int32_t scratch1 [SUP_PROC_FRAME_BINS] = {0};
    int32_t scratch2 [SUP_PROC_FRAME_BINS] = {0};
    bfp_s32_init(&sqrt_lambda, scratch1, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_init(&tmp, scratch2, INT_EXP, SUP_PROC_FRAME_BINS, 0);
    bfp_s32_sqrt(&sqrt_lambda, &state->lambda_hat);
    bfp_s32_scale(&sqrt_lambda, &sqrt_lambda, float_to_float_s32(0.25));

    bfp_s32_inverse(&tmp, &sqrt_lambda);
    bfp_s32_mul(&tmp, &tmp, abs_Y);

    float_s32_t t = bfp_s32_min(&tmp);
    comp.length = 1;
    comp.hr = 0;
    comp.flags = 0;
    comp.exp = t.exp;
    comp.data[0] = t.mant;
    bfp_s32_use_exponent(&comp, 0);

    int32_t index = (comp.data[0] > (LUT_SIZE - 1)) ? comp.data[0] : (LUT_SIZE - 1);
    t.mant = LUT[index];
    t.exp = INT_EXP;

    bfp_s32_scale(&sqrt_lambda, &sqrt_lambda, float_to_float_s32(4));
    bfp_s32_scale(&tmp, &sqrt_lambda, t);
    bfp_s32_sub(abs_Y, abs_Y, &tmp);
    bfp_s32_rect(abs_Y, abs_Y);
}

/*
 * abs_Y is the current estimated mag of Y, i.e. it may have been echo suppressed or dereverbed.
 * Y_exp is the exponent of abs_Y
 *
 */
void ns_process_frame(bfp_s32_t * abs_Y, suppression_state_t * state){

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