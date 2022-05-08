// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "vnr_features_api.h"
#include "vnr_features_priv.h"

void test_init()
{
}

void test(int32_t *output, int32_t *input) {
    //log2 output is always 8.24
    vnr_priv_log2(&output[0], (float_s32_t*)input, VNR_MEL_FILTERS);
}
