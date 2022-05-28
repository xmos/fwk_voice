// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "aec_defines.h"
#include "aec_api.h"

//AEC level 2
void aec_l2_calc_Error_and_Y_hat(
        bfp_complex_s32_t *Error,
        bfp_complex_s32_t *Y_hat,
        const bfp_complex_s32_t *Y,
        const bfp_complex_s32_t *X_fifo,
        const bfp_complex_s32_t *H_hat,
        unsigned num_x_channels,
        unsigned num_phases,
        unsigned start_offset,
        unsigned length,
        int32_t bypass_enabled)
{
    if(!length) {
        //printf("0 length\n");
        return;
    }
    if(bypass_enabled) { //Copy Y into Error. Set Y_hat to 0
        memcpy(Error->data, &Y->data[start_offset], length*sizeof(complex_s32_t));
        Error->exp = Y->exp;
        Error->hr = Y->hr;

        memset(Y_hat->data, 0, length*sizeof(complex_s32_t));
        Y_hat->exp = AEC_ZEROVAL_EXP;
        Y_hat->hr = AEC_ZEROVAL_HR;
    }
    else {
        uint32_t phases = num_x_channels * num_phases;
        for(unsigned ph=0; ph<phases; ph++) {
            //create input chunks
            bfp_complex_s32_t X_chunk, H_hat_chunk;
            bfp_complex_s32_init(&X_chunk, &X_fifo[ph].data[start_offset], X_fifo[ph].exp, length, 0); //Not recalculating headroom here to make sure outputs are bitexact irrespective of the length this function is called for.
            X_chunk.hr = X_fifo[ph].hr;
            bfp_complex_s32_init(&H_hat_chunk, &H_hat[ph].data[start_offset], H_hat[ph].exp, length, 0);
            H_hat_chunk.hr = H_hat[ph].hr;
            bfp_complex_s32_macc(Y_hat, &X_chunk, &H_hat_chunk);
        }

        bfp_complex_s32_t Y_chunk;
        bfp_complex_s32_init(&Y_chunk, &Y->data[start_offset], Y->exp, length, 0);
        Y_chunk.hr = Y->hr;
        bfp_complex_s32_sub(Error, &Y_chunk, Y_hat);
    }
}

void aec_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph
        )
{
    bfp_complex_s32_conj_macc(H_hat_ph, T_ph, X_fifo_ph);
    bfp_fft_pack_mono(H_hat_ph);
    bfp_complex_s32_gradient_constraint_mono(H_hat_ph, 240);
    bfp_fft_unpack_mono(H_hat_ph);
}

void aec_l2_bfp_complex_s32_unify_exponent(
        bfp_complex_s32_t *chunks,
        int32_t *final_exp, uint32_t *final_hr,
        const uint32_t *mapping, uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom)
{
    *final_exp = INT_MIN; 
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
            if((int32_t)(chunks[i].exp - chunks[i].hr + min_headroom) > *final_exp) {
                *final_exp = chunks[i].exp - chunks[i].hr + min_headroom;
            }
        }
    }
    *final_hr = INT_MAX; //smallest hr
    for(int i=0; i<array_len; i++) {
        if(((mapping == NULL) || (mapping[i] == desired_index)) && (chunks[i].length > 0)) {
           bfp_complex_s32_use_exponent(&chunks[i], *final_exp);
           *final_hr = (chunks[i].hr < *final_hr) ? chunks[i].hr : *final_hr;
        }
    }
}

void aec_l2_bfp_s32_unify_exponent(
        bfp_s32_t *chunks, int32_t *final_exp,
        uint32_t *final_hr,
        const uint32_t *mapping,
        uint32_t array_len,
        uint32_t desired_index,
        uint32_t min_headroom)
{
    *final_exp = INT_MIN; //find biggest exponent (fewest fraction bits) 
    for(int i=0; i<array_len; i++) {
        if((mapping == NULL) || ((mapping != NULL) && (mapping[i] == desired_index) && (chunks[i].length > 0))) {
            if((int32_t)(chunks[i].exp - chunks[i].hr + min_headroom) > *final_exp) {
                *final_exp = chunks[i].exp - chunks[i].hr + min_headroom;
            }
        }
    }
    *final_hr = INT_MAX; //smallest hr
    for(int i=0; i<array_len; i++) {
        if((mapping == NULL) || ((mapping != NULL) && (mapping[i] == desired_index) && (chunks[i].length > 0))) {
           bfp_s32_use_exponent(&chunks[i], *final_exp);
           *final_hr = (chunks[i].hr < *final_hr) ? chunks[i].hr : *final_hr;
        }
    }
}
