// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef VAD_HELPERS_H
#define VAD_HELPERS_H

#if !X86_BUILD
#include <xclib.h>
#include "dsp.h"
#else
//TODO this is just to enable the x86 to build until we have this for x86
#define clz(x) 0
#define dsp_math_logistics_fast(x) 0
#define dsp_dct_forward24(dct_output, dct_input) 0
#endif

#endif