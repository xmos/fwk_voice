// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#pragma once

#include "xmath/xmath.h"


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

//Max diff between a bfp_s32 and double precision floating point vector
C_API unsigned vector_int32_maxdiff(int32_t * B, int B_exp, double * f, int start, int count);

C_API void make_sine_table(double * sine_lut, unsigned proc_frame_length);
C_API void bit_reverse( complex_double_t pts[], const uint32_t N );
C_API void forward_fft (
    complex_double_t pts[],
    const uint32_t  N,
    const double   sine[] );
C_API void inverse_fft (
    complex_double_t pts[],
    const uint32_t  N,
    const double   sine[] );
C_API void split_spectrum( complex_double_t pts[], const uint32_t N );
C_API void merge_spectra( complex_double_t pts[], const uint32_t N );
