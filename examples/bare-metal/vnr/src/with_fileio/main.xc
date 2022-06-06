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
    void vnr_task(const char* input_filename, const char *output_filename);
}

int main (void){
    chan xscope_chan;
    par
    {
        xscope_host_data(xscope_chan);
        on tile[0]: {
            xscope_io_init(xscope_chan);
            vnr_task("input.wav", "vnr_out.bin");
            _Exit(0);        
        }
    }
    return 0;
}
