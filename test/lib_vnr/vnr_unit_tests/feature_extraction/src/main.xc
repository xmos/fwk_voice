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
void test_vnr_unit(const char *input_file_name, const char *output_file_name);
#if TEST_WAV_XSCOPE
    #include "xscope_io_device.h"
#endif
}

#define IN_FILE_NAME    "input.bin"
#define OUT_FILE_NAME   "output.bin"
int main (void)
{
  chan xscope_chan;
  par
  {
#if TEST_WAV_XSCOPE
    xscope_host_data(xscope_chan);
#endif
    on tile[0]: {
#if TEST_WAV_XSCOPE
        xscope_io_init(xscope_chan);
#endif 
        test_vnr_unit(IN_FILE_NAME, OUT_FILE_NAME);
        _Exit(0);
    }
  }
  return 0;
}
