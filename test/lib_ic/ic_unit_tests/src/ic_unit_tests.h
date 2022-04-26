// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef IC_UNIT_TESTS_
#define IC_UNIT_TESTS_

#include "unity.h"

#ifdef __XC__
#include <xs1.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <xclib.h>

#include "audio_test_tools.h"
extern "C" {
    #include "ic_state.h"
    #include "ic_low_level.h"
}

#else

#include <stdio.h>
#include <xcore/assert.h>
#include <math.h>
#include "ic_state.h"
#include "ic_low_level.h"
#include "pseudo_rand.h"

#endif // __XC__


#define TEST_ASM 1

// Set F to a power of 2 greater than 1 to speedup testing by a Fx
#undef F
#ifdef SPEEDUP_FACTOR
    #define F (SPEEDUP_FACTOR)
#else
    #define F 1
#endif


#endif /* IC_UNIT_TESTS_ */
