// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _NS_PRIV_H
#define _NS_PRIV_H

#include <ns_state.h>
#include "xmath/xmath.h"


void ns_priv_rescale_vector(bfp_complex_s32_t * Y, bfp_s32_t * new_mag, bfp_s32_t * orig_mag);

void ns_priv_pack_input(bfp_s32_t * current, const int32_t * input, bfp_s32_t * prev);

void ns_priv_apply_window(bfp_s32_t * input, 
                    bfp_s32_t * window, 
                    bfp_s32_t * rev_window, 
                    const unsigned frame_length, 
                    const unsigned window_length);

void ns_priv_form_output(int32_t * out, bfp_s32_t * in, bfp_s32_t * overlap);

void ns_priv_update_S(ns_state_t * ns, const bfp_s32_t * abs_Y);

void ns_priv_minimum(bfp_s32_t * dst, bfp_s32_t * src1, bfp_s32_t * src2);

void ns_priv_update_p(ns_state_t * ns);

void ns_priv_update_alpha_d_tilde(ns_state_t * ns);

void ns_priv_update_lambda_hat(ns_state_t * ns, const bfp_s32_t * abs_Y);

int ns_priv_update_and_test_reset(ns_state_t * ns);

void ns_priv_subtract_lambda_from_frame(bfp_s32_t * abs_Y, ns_state_t * ns);

void ns_priv_process_frame(bfp_s32_t * abs_Y, ns_state_t * ns);

#endif
