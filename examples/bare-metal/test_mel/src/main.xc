// Copyright 2022 XMOS LIMITED.
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
void test_mel(const char *in_filename, const char *mel_filename, const char *mel_exp_filename, const char *fft_filename, const char *fft_exp_filename);
#if TEST_WAV_XSCOPE
    #include "xscope_io_device.h"
#endif
}

#define IN_FILE_NAME    "data_16k/2035-152373-0002001.wav"
#define MEL_FILE_NAME   "dut_mel.bin"
#define MEL_EXP_FILE_NAME "dut_mel_exp.bin"
#define FFT_FILE_NAME   "dut_fft.bin"
#define FFT_EXP_FILE_NAME "dut_fft_exp.bin"
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
        test_mel(IN_FILE_NAME, MEL_FILE_NAME, MEL_EXP_FILE_NAME, FFT_FILE_NAME, FFT_EXP_FILE_NAME);
        _Exit(0);
    }
  }
  return 0;
}

