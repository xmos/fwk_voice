// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <xscope.h>
#include <stdlib.h>
#ifdef __XC__
#define chanend_t chanend
#else
#include <xcore/chanend.h>
#endif

extern "C" {
#include "xs3_math.h"
void main_tile0(chanend);
}

int main (void)
{
  chan xscope_chan;
  par
  {
#if TEST_WAV_XSCOPE
    xscope_host_data(xscope_chan);
#endif
    on tile[0]: {
        main_tile0(xscope_chan);
        _Exit(0);
    }
  }
  return 0;
}
