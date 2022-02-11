// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#pragma once

#include "xs3_math_conf.h"
#include "xs3_api.h"
#include "xs3_math_types.h"


/**
 * xCore: Get 100MHz reference clock timestamp
 * x86: Return 0
 */
C_API 
unsigned getTimestamp();

#define SEED_FROM_FUNC_NAME()    get_seed(__func__, sizeof(__func__))
C_API unsigned get_seed(const char* str, const unsigned len);

// Convert a double to fixed point number of a given exponent
C_API int32_t double_to_int32(double d, const int d_exp);

