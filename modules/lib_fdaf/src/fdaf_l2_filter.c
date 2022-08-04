// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <fdaf_defines.h>
#include <fdaf_api.h>

void fdaf_l2_calc_Error_and_Y_hat(
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
        Y_hat->exp = FDAF_ZEROVAL_EXP;
        Y_hat->hr = FDAF_ZEROVAL_HR;
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

void fdaf_l2_adapt_plus_fft_gc(
        bfp_complex_s32_t *H_hat_ph,
        const bfp_complex_s32_t *X_fifo_ph,
        const bfp_complex_s32_t *T_ph)
{
    bfp_complex_s32_conj_macc(H_hat_ph, T_ph, X_fifo_ph);
    bfp_fft_pack_mono(H_hat_ph);
    bfp_complex_s32_gradient_constraint_mono(H_hat_ph, 240);
    bfp_fft_unpack_mono(H_hat_ph);
}

