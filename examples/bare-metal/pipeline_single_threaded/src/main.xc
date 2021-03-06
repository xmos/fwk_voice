// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>

#include "xscope_io_device.h"

extern "C" {
    extern void pipeline_wrapper(const char *input_file_name, const char* output_file_name);
}

int main(){
    chan xscope_chan;

    par {
        xscope_host_data(xscope_chan);
        on tile[1]: 
        {
          xscope_io_init(xscope_chan);
          pipeline_wrapper("input.wav", "output.wav");
          _Exit(0);
        }
    }
    return 0;
}
