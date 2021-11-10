// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef SUP_ECHO_STATE_H
#define SUP_ECHO_STATE_H

#include <xccompat.h>

#define SIGMA_MAX (1257966796) // UINT_MAX/sqrt(2)

typedef struct{
    //config
    int adaption_enabled;
    uq8_24 mu;
    uint32_t max_suppression;

    //Note the ordering of this is carefully chosen so that it
    //works around http://bugzilla/show_bug.cgi?id=14893
    //Because SUP_X_CHANNELS may = 0
    int32_t h_hat[SUP_X_CHANNELS][SUP_PROC_FRAME_BINS];
    unsigned h_hat_hr[SUP_X_CHANNELS];
    int h_hat_exp[SUP_X_CHANNELS];

} es_state_t;

void es_limit_suppression(uint32_t * A, REFERENCE_PARAM(int, A_exp),
        int32_t * B, int B_exp,
        unsigned length, uint32_t limit);

void es_init_state(REFERENCE_PARAM(es_state_t, state));
void es_dump_parameters(REFERENCE_PARAM(es_state_t, state));

int es_calc_scale_vector_shift(int e_exp, unsigned e_hr,
        unsigned mu_hr);
void es_scale_vector(int32_t * e, unsigned e_length, uint32_t mu, int shr);

void es_convolve(uint32_t * d, unsigned d_length, uint32_t center_coef);
void es_convolve_asm(uint32_t * d, unsigned d_length, uint32_t center_coef);

int es_calc_vector_macc_shifts(int a_exp, unsigned a_hr,
        int b_exp, unsigned b_hr,
        int c_exp, unsigned c_hr,
        REFERENCE_PARAM(int, accu_shr), REFERENCE_PARAM(int, product_shr));
void es_vector_nmacc(int32_t * a, int32_t * b, uint32_t * c, int accu_shr,
        int product_shr, REFERENCE_PARAM(unsigned, mask), unsigned length);
void es_vector_macc(int32_t * a, int32_t * b, uint32_t * c, int accu_shr,
        int product_shr, REFERENCE_PARAM(unsigned, mask), unsigned length);

void es_update_error(int32_t * error, unsigned array_len, uq8_24 mu,
        REFERENCE_PARAM(unsigned, error_hr), REFERENCE_PARAM(int, error_exp));

void es_process_frame(
        uint32_t abs_Y[SUP_PROC_FRAME_BINS], REFERENCE_PARAM(int, Y_exp),
        uint32_t abs_X[SUP_X_CHANNELS][SUP_PROC_FRAME_BINS], int X_exp[SUP_X_CHANNELS],
        uint32_t inv_abs_X[SUP_X_CHANNELS][SUP_PROC_FRAME_BINS], int inv_abs_X_exp[SUP_X_CHANNELS],
        REFERENCE_PARAM(es_state_t, state));

#endif
