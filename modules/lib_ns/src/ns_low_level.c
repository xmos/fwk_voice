// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <string.h>
#include "xmath/xmath.h"
#include "ns_priv.h"
#include <ns_state.h>

#define one_mant 1073741824
#define one_exp (-30)

//    A = min (B, C) - element wise
//    A_exp = C_exp
void ns_priv_minimum(bfp_s32_t * dst, bfp_s32_t * src1, bfp_s32_t * src2){

    for (int v = 0; v < dst->length; v++){
        dst->data[v] = (src1->data[v] <= src2->data[v]) ? src1->data[v] : src2->data[v];
    }
    bfp_s32_headroom(dst);
}

//    S = alpha_s * S + (1.0 - alpha_s) * (abs(Y)**2)
void ns_priv_update_S(ns_state_t * ns, const bfp_s32_t * abs_Y){
    int32_t scratch[NS_PROC_FRAME_BINS];
    bfp_s32_t tmp;
    bfp_s32_init(&tmp, scratch, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    bfp_s32_mul(&tmp, abs_Y, abs_Y);

    bfp_s32_scale(&tmp, &tmp, ns->one_minus_alpha_s);
    
    bfp_s32_scale(&ns->S, &ns->S, ns->alpha_s);

    bfp_s32_add(&ns->S, &ns->S, &tmp);
}

//    S_r = S / S_min
//    I = S_r > delta
//    p = alpha_p * p + (1.0 - alpha_p) * I
void ns_priv_update_p(ns_state_t * ns){

    bfp_s32_t tmp, tmp2;
    int32_t scratch [NS_PROC_FRAME_BINS];
    int32_t one_zero [NS_PROC_FRAME_BINS];
    bfp_s32_init(&tmp, scratch, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    bfp_s32_scale(&tmp, &ns->S_min, ns->delta);
    bfp_s32_use_exponent(&tmp, ns->S.exp);

    for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
        one_zero[v] = (ns->S.data[v] > tmp.data[v]) ? ns->one_minus_alpha_p.mant : 0;
    }

    bfp_s32_init(&tmp2, one_zero, ns->one_minus_alpha_p.exp, NS_PROC_FRAME_BINS, 1);

    bfp_s32_scale(&ns->p, &ns->p, ns->alpha_p);
    bfp_s32_add(&ns->p, &ns->p, &tmp2);
}

//    alpha_d_tilde = alpha_d + (1.0 - alpha_d) * p
void ns_priv_update_alpha_d_tilde(ns_state_t * ns){
    bfp_s32_t tmp;
    int32_t scratch [NS_PROC_FRAME_BINS];

    bfp_s32_init(&tmp, scratch, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    bfp_s32_scale(&tmp, &ns->p, ns->one_minus_aplha_d);

    bfp_s32_add_scalar(&ns->alpha_d_tilde, &tmp, ns->alpha_d);
}

//    lambda_hat = alpha_d_tilde * lambda_hat + (1.0 - alpha_d_tilde) * (abs(Y)**2)
void ns_priv_update_lambda_hat(ns_state_t * ns, const bfp_s32_t * abs_Y){
    bfp_s32_t tmp1, tmp2;
    float_s32_t t;
    int32_t scratch1 [NS_PROC_FRAME_BINS];
    int32_t scratch2 [NS_PROC_FRAME_BINS];
    bfp_s32_init(&tmp1, scratch1, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);
    bfp_s32_init(&tmp2, scratch2, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    bfp_s32_mul(&tmp1, abs_Y, abs_Y);

    t.mant = -one_mant;
    t.exp = one_exp; // t = -1

    bfp_s32_scale(&tmp2, &ns->alpha_d_tilde, t);

    t.mant = one_mant; // t = 1

    bfp_s32_add_scalar(&tmp2, &tmp2, t);
    bfp_s32_mul(&tmp1, &tmp1, &tmp2);
    bfp_s32_mul(&ns->lambda_hat, &ns->lambda_hat, &ns->alpha_d_tilde);
    bfp_s32_add(&ns->lambda_hat, &ns->lambda_hat, &tmp1);
}

int ns_priv_update_and_test_reset(ns_state_t * ns){
    unsigned count = ns->reset_counter;
    count += NS_FRAME_ADVANCE;
    if(count > ns->reset_period){
        ns->reset_counter = count - ns->reset_period;
        return 1;
    } else {
        ns->reset_counter = count;
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
void ns_priv_subtract_lambda_from_frame(bfp_s32_t * abs_Y, ns_state_t * ns){
    bfp_s32_t sqrt_lambda, r;
    int32_t scratch1 [NS_PROC_FRAME_BINS];
    int32_t r_data [NS_PROC_FRAME_BINS];
    bfp_s32_init(&sqrt_lambda, scratch1, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

    bfp_s32_sqrt(&sqrt_lambda, &ns->lambda_hat);

    if (abs_Y->exp < sqrt_lambda.exp) {
        bfp_s32_use_exponent(abs_Y, sqrt_lambda.exp);
    }
    else {
        bfp_s32_use_exponent(&sqrt_lambda, abs_Y->exp);
    }

    for(int v = 0; v < NS_PROC_FRAME_BINS; v++){
        // 64 is a default to be rewritten
        int32_t lut_index = 64;
        const int32_t num = abs_Y->data[v];
        const int32_t den = sqrt_lambda.data[v] / LUT_INPUT_MULTIPLIER;
        if(den != 0){
            lut_index = num / den;
            r_data[v] = (lut_index > (LUT_SIZE - 1)) ? 0 : LUT[lut_index];
        }
        else {
            r_data[v] = 0;
        }

        sqrt_lambda.data[v] = (sqrt_lambda.data[v] > abs_Y->data[v]) ? abs_Y->data[v] : sqrt_lambda.data[v];
    }
    bfp_s32_headroom(&sqrt_lambda);
    bfp_s32_init(&r, r_data, NS_INT_EXP, NS_PROC_FRAME_BINS, 1);

    bfp_s32_mul(&r, &sqrt_lambda, &r);
    bfp_s32_sub(abs_Y, abs_Y, &r);
    bfp_s32_rect(abs_Y, abs_Y);
}

//    abs_Y is the current estimation magnitude of Y
//    this function will estimate the noise level 
//    and subrtact it from abs_Y
void ns_priv_process_frame(bfp_s32_t * abs_Y, ns_state_t * ns){

    ns_priv_update_S(ns, abs_Y);

    bfp_s32_use_exponent(&ns->S_min, ns->S.exp);
    bfp_s32_use_exponent(&ns->S_tmp, ns->S.exp);

    if(ns_priv_update_and_test_reset(ns)){
        ns_priv_minimum(&ns->S_min, &ns->S_tmp, &ns->S);
        memcpy(ns->S_tmp.data, ns->S.data, sizeof(int32_t) * NS_PROC_FRAME_BINS);
        ns->S_tmp.exp = ns->S.exp;
        ns->S_tmp.hr = ns->S.hr;
    } else {
        ns_priv_minimum(&ns->S_min, &ns->S_min, &ns->S);
        ns_priv_minimum(&ns->S_tmp, &ns->S_tmp, &ns->S);
    }

    ns_priv_update_p(ns);

    ns_priv_update_alpha_d_tilde(ns);

    ns_priv_update_lambda_hat(ns, abs_Y);

    ns_priv_subtract_lambda_from_frame(abs_Y, ns);

}
