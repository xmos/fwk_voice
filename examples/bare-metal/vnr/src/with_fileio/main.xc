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
#include "xscope_io_device.h"


extern "C"{
    void get_frame(chanend_t c_get_frame, const char* in_filename);
    void vnr(chanend, chanend);
    void send_frame(chanend_t c_send_frame, const char* out_filename);
}

int main (void){
    chan xscope_chan;
    chan read_chan;
    chan write_chan;
    par {
        xscope_host_data(xscope_chan);
        on tile[0]: {
            xscope_io_init(xscope_chan);
            get_frame(read_chan, "input.wav");
            _Exit(0);        
        }
        on tile[0]: vnr(read_chan, write_chan);
        on tile[0]: send_frame(write_chan, "vnr_out.bin");
    }
    return 0;
}
