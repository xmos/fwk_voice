// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
//#include <stdint.h>
//#include <limits.h>
#include <string.h>
#include <bfp_math.h>
#include <stdio.h>

#include <sup_state.h>
#include "sup_ns.h"
#include "sup_test.h"

#define one_mant 1073741824
#define one_exp -30

void ns_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main){

    bfp_s32_use_exponent(A, main->exp);
    bfp_s32_use_exponent(B, main->exp);
}

//    A = min (B, C) - element wise
//    A_exp = C_exp
void ns_minimum(bfp_s32_t * dst, bfp_s32_t * src1, bfp_s32_t * src2){

    for (int v = 0; v < dst->length; v++){
        dst->data[v] = (src1->data[v] <= src2->data[v]) ? src1->data[v] : src2->data[v];
    }
    bfp_s32_headroom(dst);
}

//    S = alpha_s * S + (1.0 - alpha_s) * (abs(Y)**2)
void ns_update_S(sup_state_t *state, const bfp_s32_t * abs_Y){
    int32_t scratch[SUP_PROC_FRAME_BINS];
    bfp_s32_t tmp;
    bfp_s32_init(&tmp, scratch, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_mul(&tmp, abs_Y, abs_Y);

    //since alpha_s = 0.8 and 1 - 0.8 = 0.2, we can use alpha_p value here
    bfp_s32_scale(&tmp, &tmp, state->alpha_p);
    
    bfp_s32_scale(&state->S, &state->S, state->alpha_s);

    bfp_s32_add(&state->S, &state->S, &tmp);
}

//    S_r = S / S_min
//    I = S_r > delta
//    p = alpha_p * p + (1.0 - alpha_p) * I
void ns_update_p(sup_state_t * state){

    bfp_s32_t tmp, tmp2;
    int32_t scratch [SUP_PROC_FRAME_BINS];
    int32_t one_zero [SUP_PROC_FRAME_BINS];
    bfp_s32_init(&tmp, scratch, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_scale(&tmp, &state->S_min, state->delta);
    bfp_s32_use_exponent(&tmp, state->S.exp);

    //since alpha_p = 0.2 and 1 - 0.2 = 0.8, we can use alpha_s value here
    for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
        one_zero[v] = (state->S.data[v] > tmp.data[v]) ? state->alpha_s.mant : 0;
    }

    bfp_s32_init(&tmp2, one_zero, state->alpha_s.exp, SUP_PROC_FRAME_BINS, 1);

    bfp_s32_scale(&state->p, &state->p, state->alpha_p);
    bfp_s32_add(&state->p, &state->p, &tmp2);
}

//    alpha_d_tilde = alpha_d + (1.0 - alpha_d) * p
void ns_update_alpha_d_tilde(sup_state_t * state){
    bfp_s32_t tmp;
    int32_t scratch [SUP_PROC_FRAME_BINS];

    bfp_s32_init(&tmp, scratch, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_scale(&tmp, &state->p, state->one_minus_aplha_d);

    bfp_s32_add_scalar(&state->alpha_d_tilde, &tmp, state->alpha_d);
}

//    lambda_hat = alpha_d_tilde * lambda_hat + (1.0 - alpha_d_tilde) * (abs(Y)**2)
void ns_update_lambda_hat(sup_state_t * state, const bfp_s32_t * abs_Y){
    bfp_s32_t tmp1, tmp2;
    float_s32_t t;
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
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1468389691, 1468389691, 1468389691, 1468389691,
1139054005, 883582904, 685409774, 531683621,
412435719, 319933163, 248177410, 192515294,
149337276, 115843378, 89861612, 69707128,
54072965, 41945287, 32537648, 25239988,
19579073, 15187809, 11781433, 9139051,
7089312, 5499296, 4265894, 3309124,
2566942, 1991219, 1544621, 1198188,
929454, 720993, 559286, 433847,
336543, 261062, 202510, 157090,
121857, 94527, 73326, 56880,
44123, 34227, 26550, 20596,
15976, 12393, 9614, 7457
};

//    sqrt_lamda = sqrt(lamda_hat)
//    denom = sqrt_lamda / lut_input_multiplier
//    lut_index = input_spectrum / denom
//    lut_index = min(lut_index, len(LUT) - 1)
//    r = LUT[lut_index]
//    desired_mag_limited =  max(input_spectrum  - (sqrt_lamda * r), 0)
void ns_subtract_lambda_from_frame(bfp_s32_t * abs_Y, sup_state_t * state){
    bfp_s32_t sqrt_lambda, r;
    int32_t scratch1 [SUP_PROC_FRAME_BINS];
    int32_t r_data [SUP_PROC_FRAME_BINS];
    bfp_s32_init(&sqrt_lambda, scratch1, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 0);

    bfp_s32_sqrt(&sqrt_lambda, &state->lambda_hat);

    if (abs_Y->exp < sqrt_lambda.exp) bfp_s32_use_exponent(abs_Y, sqrt_lambda.exp);
    else bfp_s32_use_exponent(&sqrt_lambda, abs_Y->exp);

    for(int v = 0; v < SUP_PROC_FRAME_BINS; v++){
        int32_t num, den, lut_index;
        num = abs_Y->data[v];
        den = sqrt_lambda.data[v] / LUT_INPUT_MULTIPLIER;
        if(den != 0){
            lut_index = num / den;
            r_data[v] = (lut_index > (LUT_SIZE - 1)) ? 0 : LUT[lut_index];
        }
        else r_data[v] = 0;

        sqrt_lambda.data[v] = (sqrt_lambda.data[v] > abs_Y->data[v]) ? abs_Y->data[v] : sqrt_lambda.data[v];
    }
    bfp_s32_headroom(&sqrt_lambda);
    bfp_s32_init(&r, r_data, SUP_INT_EXP, SUP_PROC_FRAME_BINS, 1);

    bfp_s32_mul(&r, &sqrt_lambda, &r);
    bfp_s32_sub(abs_Y, abs_Y, &r);
    bfp_s32_rect(abs_Y, abs_Y);
}

//    abs_Y is the current estimation magnitude of Y
//    this function will estimate the noise level 
//    and subrtact it from abs_Y
void ns_process_frame(bfp_s32_t * abs_Y, sup_state_t * state){

    ns_update_S(state, abs_Y);

    ns_adjust_exp(&state->S_min, &state->S_tmp, &state->S);

    if(ns_update_and_test_reset(state)){
        ns_minimum(&state->S_min, &state->S_tmp, &state->S);
        memcpy(state->S_tmp.data, state->S.data, sizeof(int32_t) * SUP_PROC_FRAME_BINS);
        state->S_tmp.exp = state->S.exp;
        state->S_tmp.hr = state->S.hr;
    } else {
        ns_minimum(&state->S_min, &state->S_min, &state->S);
        ns_minimum(&state->S_tmp, &state->S_tmp, &state->S);
    }

    ns_update_p(state);

    ns_update_alpha_d_tilde(state);

    ns_update_lambda_hat(state, abs_Y);

    ns_subtract_lambda_from_frame(abs_Y, state);

}