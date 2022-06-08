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

extern "C"{
  void get_frame(chanend, chanend);
  void vnr(chanend, chanend);
  void send_frame(chanend);
}

int main (void){
  chan xscope_chan;
  chan read_chan;
  chan write_chan;
  par {
    xscope_host_data(xscope_chan);
    on tile[0]: get_frame(xscope_chan, read_chan);
    on tile[0]: vnr(read_chan, write_chan);
    on tile[0]: send_frame(write_chan);
  }
  return 0;
}
