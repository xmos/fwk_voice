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
void vnr_task(const char *feature_in_filename, const char *feature_out_filename, const char *inference_out_filename);
#if TEST_WAV_XSCOPE
    #include "xscope_io_device.h"
#endif
}

#define FEATURE_IN_FILE_NAME    "features.bin"
#define FEATURE_LOOPBACK_FILE_NAME   "loopback_features.bin"
#define INFERENCE_OUTPUT_FILE_NAME "xcore_inference.bin"
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
        vnr_task(FEATURE_IN_FILE_NAME, FEATURE_LOOPBACK_FILE_NAME, INFERENCE_OUTPUT_FILE_NAME);
        _Exit(0);
    }
  }
  return 0;
}
