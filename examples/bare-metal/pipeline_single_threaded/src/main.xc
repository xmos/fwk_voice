// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include <xscope.h>
#include <stdlib.h>

#include "xscope_io_device.h"

extern "C" {
    extern void pipeline_wrapper_tile0(chanend c_pcm_out, chanend c_pcm_in, const char *input_file_name, const char* output_file_name);
    extern void pipeline_wrapper_tile1(chanend c_pcm_in, chanend c_pcm_out);
}

int main(){
    chan xscope_chan;
    chan c_tile0_to_tile1;
    chan c_tile1_to_tile0;

    par {
        xscope_host_data(xscope_chan);
        on tile[0]: 
        {
          xscope_io_init(xscope_chan);
          pipeline_wrapper_tile0(c_tile0_to_tile1, c_tile1_to_tile0, "input.wav", "output.wav");
          _Exit(0);
        }
        on tile[1]:
        {
            pipeline_wrapper_tile1(c_tile0_to_tile1, c_tile1_to_tile0);
        }
    }
    return 0;
}
