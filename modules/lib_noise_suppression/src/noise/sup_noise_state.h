// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdint.h>
#include <suppression.h>
#include <bfp_s32.h>

#ifndef SUP_NOISE_STATE_H
#define SUP_NOISE_STATE_H

void adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main);

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
