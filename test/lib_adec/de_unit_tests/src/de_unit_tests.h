// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef DE_UNIT_TESTS_
#define DE_UNIT_TESTS_

#include "unity.h"


#include <string.h>
#include <math.h>

#include "pseudo_rand.h"
#include "aec_defines.h"
#include "aec_config.h"
#include "aec_memory_pool.h"

// Set F to a power of 2 greater than 1 to speedup testing by a Fx
#undef F
#if SPEEDUP_FACTOR
    #define F (SPEEDUP_FACTOR)
#else
    #define F 1
#endif


#endif /* DE_UNIT_TESTS_ */
