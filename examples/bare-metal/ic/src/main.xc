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
#include "xmath/xmath.h"
void main_tile0(chanend, chanend);
void main_tile1(chanend);
}

int main (void)
{
  chan c_cross_tile, xscope_chan;
  par
  {
#if TEST_WAV_XSCOPE
    xscope_host_data(xscope_chan);
#endif
    on tile[0]: {
        main_tile0(c_cross_tile, xscope_chan);
        _Exit(0);
    }
    on tile[1]: main_tile1(c_cross_tile);
  }
  return 0;
}
