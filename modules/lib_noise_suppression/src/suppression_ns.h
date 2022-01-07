// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _suppression_ns_h
#define _suppression_ns_h

#include <suppression_state.h>

void ns_adjust_exp(bfp_s32_t * A, bfp_s32_t *B, bfp_s32_t * main);

void ns_process_frame(bfp_s32_t * abs_Y, sup_state_t * state);

#endif