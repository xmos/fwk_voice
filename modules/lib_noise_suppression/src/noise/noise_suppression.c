// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <bfp_init.h>

//#include <xclib.h>

#include <suppression.h>

#if SUP_DEBUG | SUP_DEBUG_PRINT | SUP_WARNING_PRINT
#include <stdio.h>
#include "audio_test_tools.h"
#include <math.h>
#endif


uint32_t sqrt30_xs2(uint32_t x);
uint32_t hypot_i(int32_t a, int32_t b, unsigned c);

void sup_set_noise_floor(suppression_state_t * state, int32_t noise_floor){

    state->noise_floor = noise_floor;

}

void sup_set_noise_delta(suppression_state_t * state, int32_t delta){

    state->delta_q24 = delta;

}

void sup_set_noise_alpha_p(suppression_state_t * state, int32_t alpha_p){

    state->alpha_p = alpha_p;

}

void sup_set_noise_alpha_d(suppression_state_t * state, int32_t alpha_d){

    state->alpha_d = alpha_d;

}

void sup_set_noise_alpha_s(suppression_state_t * state, int32_t alpha_s){

    state->alpha_s = alpha_s;

}

/*
void ns_dump_parameters(ns_state_t * state){
#if SUP_DEBUG_PRINT
    printf("NS reset_period: %d samples\n", state->reset_period);
    printf("NS alpha_d: %f\n", att_uint32_to_double(state->alpha_d, -32));
    printf("NS alpha_s: %f\n", att_uint32_to_double(state->alpha_s, -32));
    printf("NS alpha_p: %f\n", att_uint32_to_double(state->alpha_p, -32));
    printf("NS noise_floor: %f\n", att_uint32_to_double(state->noise_floor, -32));
    printf("NS delta: %f\n", att_uint32_to_double(state->delta, -32));
    printf("NS alpha_gain: %f\n", att_uint32_to_double(state->alpha_gain, -32));
#endif
}

void ns_init_state1(ns_state_t1 * state){
    memset(&state, 0, sizeof(state));
    state->reset_counter = 0;

    state->S_exp = -32;
    state->S_headroom = 0;

    for(unsigned i=0;i<sizeof(state->S_min)/sizeof(state->S_min[0]);i++){
        state->S_min[i] = UINT_MAX;
        state->S_tmp[i] = UINT_MAX;
        state->S[i] = UINT_MAX;
        state->p[i] = 0;
        state->alpha_d_tilde[i] = 0;
        state->bin_gain[i] = UINT_MAX;
        state->lambda_hat[i] = 0;
    }
#if SUP_DEBUG

    if(!att_is_double_word_aligned((int *)state->S_min))
        printf("Error: data not aligned state.S_min %p\n", state->S_min);
    if(!att_is_double_word_aligned((int *)state->S_tmp))
        printf("Error: data not aligned state.S_tmp %p\n", state->S_tmp);
    if(!att_is_double_word_aligned((int *)state->S))
        printf("Error: data not aligned state.S %p\n", state->S);
    if(!att_is_double_word_aligned((int *)state->p))
        printf("Error: data not aligned state.p %p\n", state->p);
    if(!att_is_double_word_aligned((int *)state->alpha_d_tilde))
        printf("Error: data not aligned state.alpha_d_tilde %p\n", state->alpha_d_tilde);
    if(!att_is_double_word_aligned((int *)state->bin_gain))
        printf("Error: data not aligned state.bin_gain %p\n", state->bin_gain);
    if(!att_is_double_word_aligned((int *)state->lambda_hat))
        printf("Error: data not aligned state.lambda_hat %p\n", state->lambda_hat);
    if(!att_is_double_word_aligned((int *)state->prev_frame))
        printf("Error: data not aligned state.prev_frame %p\n", state->prev_frame);
#endif
    state->lambda_hat_exp = -1024;
    state->lambda_hat_headroom = 31;
    
    state->reset_period = (unsigned)(16000.0 * 0.15);
    state->alpha_d = (uint32_t)((double)UINT_MAX * 0.95);
    state->alpha_s = (uint32_t)((double)UINT_MAX * 0.8);
    state->alpha_p = (uint32_t)((double)UINT_MAX * 0.2);
    state->noise_floor = (uint32_t)((double)UINT_MAX * 0.1258925412); // -18dB
    state->delta = Q24(1.5);
    state->alpha_gain = UINT_MAX;
}

void ns_init_state0(ns_state_t * state){
    memset(&state, 0, sizeof(state));
    state->reset_counter = 0;
    int32_t scrutch_S[SUP_PROC_FRAME_BINS] = {INT_MAX}, scrutch_S_min[SUP_PROC_FRAME_BINS] = {INT_MAX}, scrutch_S_tmp[SUP_PROC_FRAME_BINS] = {INT_MAX};
    int32_t scrutch_lambda_hat[SUP_PROC_FRAME_BINS] = {0}, scrutch_p[SUP_PROC_FRAME_BINS] = {0};
    int32_t scrutch_bin_gain[SUP_PROC_FRAME_BINS] = {INT_MAX}, scrutch_alpha_d_title[SUP_PROC_FRAME_BINS] = {0};

    bfp_s32_init(state->S.data, scrutch_S,-32, SUP_PROC_FRAME_BINS, 1);
    bfp_s32_init(state->S_min.data, scrutch_S_min,-32, SUP_PROC_FRAME_BINS, 1);
    bfp_s32_init(state->S_tmp.data, scrutch_S_tmp,-32, SUP_PROC_FRAME_BINS, 1);
    bfp_s32_init(state->lambda_hat.data, scrutch_lambda_hat, -1024, SUP_PROC_FRAME_BINS, 1);

    bfp_s32_init(state->p.data, scrutch_p, -31, SUP_PROC_FRAME_BINS, 1);
    bfp_s32_init(state->bin_gain.data, scrutch_bin_gain, -31, SUP_PROC_FRAME_BINS, 1);
    bfp_s32_init(state->alpha_d_tilde.data, scrutch_alpha_d_title, -31, SUP_PROC_FRAME_BINS, 1);

#if SUP_DEBUG

    if(!att_is_double_word_aligned((int *)state->S_min))
        printf("Error: data not aligned state.S_min %p\n", state->S_min);
    if(!att_is_double_word_aligned((int *)state->S_tmp))
        printf("Error: data not aligned state.S_tmp %p\n", state->S_tmp);
    if(!att_is_double_word_aligned((int *)state->S))
        printf("Error: data not aligned state.S %p\n", state->S);
    if(!att_is_double_word_aligned((int *)state->p))
        printf("Error: data not aligned state.p %p\n", state->p);
    if(!att_is_double_word_aligned((int *)state->alpha_d_tilde))
        printf("Error: data not aligned state.alpha_d_tilde %p\n", state->alpha_d_tilde);
    if(!att_is_double_word_aligned((int *)state->bin_gain))
        printf("Error: data not aligned state.bin_gain %p\n", state->bin_gain);
    if(!att_is_double_word_aligned((int *)state->lambda_hat))
        printf("Error: data not aligned state.lambda_hat %p\n", state->lambda_hat);
    if(!att_is_double_word_aligned((int *)state->prev_frame))
        printf("Error: data not aligned state.prev_frame %p\n", state->prev_frame);
#endif
    
    state->reset_period = (unsigned)(16000.0 * 0.15);
    state->alpha_d = (int32_t)((double)INT_MAX * 0.95);
    state->alpha_s = (int32_t)((double)INT_MAX * 0.8);
    state->alpha_p = (int32_t)((double)INT_MAX * 0.2);
    state->noise_floor = (int32_t)((double)INT_MAX * 0.1258925412); // -18dB
    state->delta = Q24(1.5);
    state->alpha_gain = INT_MAX;
}
*/


void sup_reset_noise_suppression(suppression_state_t * sup) {
    for(uint32_t j = 0; j < SUP_Y_CHANNELS; j++) {
        sup->reset_counter = 0;
        sup->S_exp = -1024;
        sup->S_headroom = 31;
        for(unsigned i=0;i<sizeof(sup->ns[j].S_min)/sizeof(sup->ns[j].S_min[0]);i++){
            sup->ns[j].S_min[i] = UINT_MAX;
            sup->ns[j].S_tmp[i] = UINT_MAX;
            sup->ns[j].S[i] = UINT_MAX;
            sup->ns[j].p[i] = 0;
        }

        sup->ns[j].lambda_hat_exp = -1024;
        sup->ns[j].lambda_hat_headroom = 31;
    }
}


void sup_set_noise_suppression_enable(suppression_state_t * sup, int enable) {
    sup->noise_suppression_enabled = enable;
}


// A = min (B, C) - element wise
// A_exp = C_exp
void ns_minimum(uint32_t * alias A, uint32_t * alias B, uint32_t * alias C,
        unsigned length, int B_exp, int C_exp){

    if(C_exp < B_exp){
        int delta_exp = B_exp - C_exp;
        for(unsigned i=0;i<length;i++){
            if((C[i]>>delta_exp) < B[i]){
                A[i] = C[i];
            } else {
                A[i] = B[i]<<delta_exp;
            }
        }
    } else {
        int delta_exp = C_exp - B_exp;
        for(unsigned i=0;i<length;i++){
            if(C[i] < (B[i]>>delta_exp)){
                A[i] = C[i];
            } else {
                A[i] = B[i]>>delta_exp;
            }
        }
    }
}


//S = alpha_s*S + (1.0-alpha_s)*(abs(Y)**2)
void ns_update_S(int32_t * abs_Y, unsigned length, int Y_exp,
        ns_state_t * state){

      uint32_t alpha_s = state->alpha_s;
      int absYsquared_exp = 2*Y_exp +32;

      unsigned S_mask = 0;

      //This is to compensate for the extra headroom
      int adjusted_S_exp = state->S_exp - state->S_headroom;

      if(absYsquared_exp < adjusted_S_exp){
          unsigned s = adjusted_S_exp - absYsquared_exp;
          for(unsigned i=0;i<length;i++){
             uint64_t tmp = (int64_t)abs_Y[i] * (int64_t)abs_Y[i];
             uint32_t absYsquared = tmp>>32;
             uint32_t left = ((uint64_t)state->S[i]*(uint64_t)alpha_s)>>(32 - state->S_headroom);
             uint32_t right = ((uint64_t)absYsquared*(uint64_t)(UINT_MAX - alpha_s))>>(s+32);
             state->S[i] = left + right;
             S_mask |= state->S[i];
          }
          state->S_exp = adjusted_S_exp;
      } else {
          unsigned s = absYsquared_exp - adjusted_S_exp;
          for(unsigned i=0;i<length;i++){
             uint64_t tmp = (int64_t)abs_Y[i] * (int64_t)abs_Y[i];
             uint32_t absYsquared = tmp>>32;

             uint32_t left = ((uint64_t)state->S[i]*(uint64_t)alpha_s)>>(s + 32 - state->S_headroom);
             uint32_t right = ((uint64_t)absYsquared*(uint64_t)(UINT_MAX - alpha_s))>>(32);
             state->S[i] = left + right;
             S_mask |= state->S[i];
          }
          state->S_exp = absYsquared_exp;
      }
      state->S_headroom = clz(S_mask);
}

//    S_r = S/S_min
//    I = S_r > delta
//    p = alpha_p * p + (1.0 - alpha_p) * I
void ns_update_p(ns_state_t * state, unsigned length){

    uint32_t alpha_p = state->alpha_p;
    uint64_t delta = (uint64_t)state->delta;

    for(unsigned i=0;i<length;i++){
        uint32_t t = (delta * (uint64_t)state->S_min[i])>>32;
        state->p[i] = ((uint64_t)state->p[i] * (uint64_t)alpha_p)>>32;
        if((state->S[i]>>8) > t){ //8 because delta is a q24
            state->p[i] += (UINT_MAX - alpha_p);
        }
    }
}

//    alpha_d_tilde = alpha_d + (1.0 - alpha_d) * p
void ns_update_alpha_d_tilde(uint32_t p[], uint32_t alpha_d_tilde[],
        uint32_t alpha_d, unsigned length){
    for(unsigned i=0;i<length;i++){
        uint32_t t = ((uint64_t)p[i] * (uint64_t)(UINT_MAX - alpha_d))>>32;
        alpha_d_tilde[i] = alpha_d + t;
    }
}

//    lambda_hat = alpha_d_tilde[i]*lambda_hat + (1.0 - alpha_d_tilde)*np.square(np.absolute(Y))
void ns_update_lambda_hat(uint32_t * abs_Y, unsigned length, int Y_exp,
        ns_state_t * state){

    int absYsquared_exp = 2*Y_exp + 32+1;

    unsigned mask = 0;

    //This is to compensate for the extra headroom
    int adjusted_S_exp = state->lambda_hat_exp - state->lambda_hat_headroom;

    if(absYsquared_exp < adjusted_S_exp){
        unsigned s = adjusted_S_exp - absYsquared_exp;
        for(unsigned i=0;i<length;i++){
           int64_t tmp = (int64_t)abs_Y[i] * (int64_t)abs_Y[i];
           uint32_t absYsquared = tmp>>32;
           uint32_t alpha = state->alpha_d_tilde[i];
           uint32_t left = ((uint64_t)state->lambda_hat[i]*(uint64_t)alpha)>>(32 - state->lambda_hat_headroom);
           uint32_t right = ((uint64_t)absYsquared*(uint64_t)(UINT_MAX - alpha))>>(s+32+1);
           state->lambda_hat[i] = left + right;
           mask |= state->lambda_hat[i];
        }
        state->lambda_hat_exp = adjusted_S_exp;
    } else {
        unsigned s = absYsquared_exp - adjusted_S_exp;
        for(unsigned i=0;i<length;i++){
           int64_t tmp = (int64_t)abs_Y[i] * (int64_t)abs_Y[i];
           uint32_t absYsquared = tmp>>32;
           uint32_t alpha = state->alpha_d_tilde[i];
           uint32_t left = ((uint64_t)state->lambda_hat[i]*(uint64_t)alpha)>>(s + 32 - state->lambda_hat_headroom);
           uint32_t right = ((uint64_t)absYsquared*(uint64_t)(UINT_MAX - alpha))>>(32+1);
           state->lambda_hat[i] = left + right;
           mask |= state->lambda_hat[i];
        }
        state->lambda_hat_exp = absYsquared_exp;
    }
    state->lambda_hat_headroom = clz(mask);
}

/*
int ns_update_and_test_reset(ns_state_t * state){
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
*/

//{int, int} sqrt_calc_exp(const int exp, const  unsigned hr){
//    int even_exp = exp - hr;
//    if(even_exp&1){
//        even_exp +=1;
//        if(hr){
//            return {hr -1, even_exp/2 - 14};
//        } else {
//            return {-1, even_exp/2 - 14};
//        }
//    } else {
//        return {hr, even_exp/2 -14};
//    }
//}
//
//uint32_t sqrt30_xs2(uint32_t x);
//
//void ns_sqrt(uint32_t &s, int &exp, const unsigned hr){
//
//    int shl, output_exp;
//    {shl, output_exp} = sqrt_calc_exp(exp, hr);
//    if(shl > 0)
//        s = sqrt30_xs2(s<<shl);
//    else
//        s = sqrt30_xs2(s>>(-shl));
//    exp = output_exp;
//}

#define HYPOT_I_PRECISION (16)
static uint32_t shr32(uint32_t a, int v){
    if (v > 0) return a >> v;
    else return a << (-v);
}


#define LUT_SIZE 64
#define LUT_INPUT_MULTIPLIER 4
const uint32_t LUT[LUT_SIZE] = {
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

/*
void ns_subtract_lambda_from_frame(uint32_t * abs_Y, unsigned length, int * Y_exp,
        ns_state_t * state){

    int lambda_exp = state->lambda_hat_exp;
    unsigned lambda_hr = state->lambda_hat_headroom;

    int lambda_shl, sqrt_lambda_exp;
    {lambda_shl, sqrt_lambda_exp} = vtb_sqrt_calc_exp(lambda_exp, lambda_hr);

    int abs_Y_exp = Y_exp;
    int abs_Y_shr = 0, sqrt_lambda_shr = 0;

    int m_exp;  // This is going to be the sum of abs_Y and -sqrt_lambda

    if(abs_Y_exp < sqrt_lambda_exp){
        m_exp = sqrt_lambda_exp;
        abs_Y_shr = sqrt_lambda_exp - abs_Y_exp;
    } else {
        m_exp = abs_Y_exp;
        sqrt_lambda_shr = abs_Y_exp - sqrt_lambda_exp;
    }
    abs_Y_shr += 1;
    sqrt_lambda_shr += 1;
    m_exp +=1;

    for(unsigned i=0;i<length;i++){

        uint32_t sqrt_lambda;
        if(lambda_shl >= 0)
            sqrt_lambda = sqrt30_xs2(state.lambda_hat[i]<<lambda_shl);
        else
            sqrt_lambda = sqrt30_xs2(state.lambda_hat[i]>>1);

        uint32_t abs_Y_norm = shr32(abs_Y[i], abs_Y_shr);
        sqrt_lambda = shr32(sqrt_lambda, sqrt_lambda_shr);

        uint32_t r;

        uint32_t denom = (sqrt_lambda/LUT_INPUT_MULTIPLIER);

        if(denom != 0){

            uint32_t numerator = abs_Y_norm;
            unsigned lut_index = numerator / denom;
            if(lut_index > (LUT_SIZE-1)){
                r = 0;
            } else {
                r = LUT[lut_index];
            }
        } else {
            r=0;
        }

        if(sqrt_lambda > abs_Y_norm){
            sqrt_lambda = abs_Y_norm;
        } 

        uint32_t sqrt_lambda_limited = ((uint64_t) sqrt_lambda*(uint64_t)r)>>32;

        abs_Y[i] = abs_Y_norm - sqrt_lambda_limited;
        
        
    }
    Y_exp = m_exp;
}
*/


/*
 * abs_Y is the current estimated mag of Y, i.e. it may have been echo suppressed or dereverbed.
 * Y_exp is the exponent of abs_Y
 *
 */
void ns_process_frame(uint32_t abs_Y[SUP_PROC_FRAME_BINS], int &Y_exp, ns_state_t * state){
/*
#if SUP_DEBUG_PRINT
    printf("abs_Y_entry = ");att_print_python_uint32(abs_Y, SUP_PROC_FRAME_BINS, Y_exp);
    printf("before_S = ");att_print_python_uint32(state.S, SUP_PROC_FRAME_BINS, state.S_exp);
    printf("before_S_min = ");att_print_python_uint32(state.S_min, SUP_PROC_FRAME_BINS, state.S_exp);
    printf("before_S_tmp = ");att_print_python_uint32(state.S_tmp, SUP_PROC_FRAME_BINS, state.S_exp);
#endif
*/

    int original_S_exp = state->S.exp;

    ns_update_S(abs_Y, SUP_PROC_FRAME_BINS, Y_exp, state);
/*
#if SUP_DEBUG_PRINT
    printf("S = ");att_print_python_uint32(state.S, SUP_PROC_FRAME_BINS, state.S_exp);
#endif
*/
    if(ns_update_and_test_reset(state)){
        ns_minimum(state.S_min, state->S_tmp, state.S, SUP_PROC_FRAME_BINS, original_S_exp, state.S_exp);
        memcpy(state.S_tmp, state->S, sizeof(state.S));
    } else {
        ns_minimum(state.S_min, state->S_min, state.S, SUP_PROC_FRAME_BINS, original_S_exp, state.S_exp);
        ns_minimum(state.S_tmp, state->S_tmp, state.S, SUP_PROC_FRAME_BINS, original_S_exp, state.S_exp);
    }
/*
#if SUP_DEBUG_PRINT
    printf("S_min = ");att_print_python_uint32(state.S_min, SUP_PROC_FRAME_BINS, state.S_exp);
    printf("S_tmp = ");att_print_python_uint32(state.S_tmp, SUP_PROC_FRAME_BINS, state.S_exp);
#endif

#if SUP_DEBUG_PRINT
    printf("p = ");att_print_python_uint32(state.p, SUP_PROC_FRAME_BINS, 0);
#endif
*/
    ns_update_p(state, SUP_PROC_FRAME_BINS);

/*
#if SUP_DEBUG_PRINT
    printf("p = ");att_print_python_uint32(state.p, SUP_PROC_FRAME_BINS, 0);
#endif

#if SUP_DEBUG_PRINT
    printf("alpha_d_tilde_before = ");att_print_python_uint32(state.alpha_d_tilde, SUP_PROC_FRAME_BINS, -32);
#endif
*/
    ns_update_alpha_d_tilde_asm(state.p, state.alpha_d_tilde, state.alpha_d, SUP_PROC_FRAME_BINS);
/*
#if SUP_DEBUG_PRINT
    printf("alpha_d_tilde = ");att_print_python_uint32(state.alpha_d_tilde, SUP_PROC_FRAME_BINS, -32);
#endif
*/
    ns_update_lambda_hat(abs_Y, SUP_PROC_FRAME_BINS, Y_exp, state);

/*
#if SUP_DEBUG_PRINT
    printf("lambda_hat = ");att_print_python_uint32(state.lambda_hat, SUP_PROC_FRAME_BINS, state.lambda_hat_exp);
#endif
*/
    ns_subtract_lambda_from_frame(abs_Y, SUP_PROC_FRAME_BINS, Y_exp, state);
/*
#if SUP_DEBUG_PRINT
    printf("abs_Y_end = ");att_print_python_uint32(abs_Y, SUP_PROC_FRAME_BINS, Y_exp);
#endif
*/
}