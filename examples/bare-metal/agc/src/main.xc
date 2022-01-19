// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <xscope.h>
#include <stdlib.h>

extern "C" {
#if TEST_WAV_XSCOPE
#include <xscope_io_device.h>
#endif

void agc_task(const char *input_file_name, const char *output_file_name);
}

#define chanend_t chanend

#define IN_WAV_FILE_NAME "input.wav"
#define OUT_WAV_FILE_NAME "output.wav"

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
        agc_task(IN_WAV_FILE_NAME, OUT_WAV_FILE_NAME);
        _Exit(0);
    }
  }
  return 0;
}
