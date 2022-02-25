// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _NS_PREV_H
#define _NS_PREV_H

#include <ns_state.h>

void ns_priv_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main);

void ns_priv_process_frame(bfp_s32_t * abs_Y, ns_state_t * state);

#endif