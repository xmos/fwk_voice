// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <xscope.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>
#include "profile.h"

static uint64_t profile_checkpoint[N_PROFILE_POINTS] = {0};
static uint32_t profile_checkpoint_count[N_PROFILE_POINTS] = {0};

void prof(unsigned n, char *str)
{
    profile_checkpoint[n] += (uint64_t)get_reference_time();
    profile_checkpoint_count[n] += 1;
}

void print_prof(unsigned start, unsigned end, unsigned frame) {
    printf("frame %d\n",frame);
    for(int i=start;i<end;i++)
    {
        if(profile_checkpoint[i] != 0) {
            printf("Profile %d, %llu\n", i, profile_checkpoint[i]);
        }
    }
    
    memset(profile_checkpoint, 0, sizeof(profile_checkpoint));
    memset(profile_checkpoint_count, 0, sizeof(profile_checkpoint_count));
}
