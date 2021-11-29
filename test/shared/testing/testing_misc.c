// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// XMOS Public License: Version 1

#include "testing.h"

#include <math.h>
#include <stdio.h>

#ifdef __xcore__
 #include "xcore/hwtimer.h"
#endif


unsigned get_seed(
    const char* str, 
    const unsigned len)
{
  unsigned seed = 0;
  int left = len;

  while(left > 0){
    unsigned v = ((unsigned*)str)[0];
    seed = seed ^ v;
    left -= 4;
    str = &str[4];
  }

  return seed;
}

unsigned getTimestamp()
{
#if __xcore__
  return get_reference_time();
#else
  return 0;
#endif
}
