// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>

#include <xscope.h>

extern "C" {
    void main_tile0(chanend);
    void main_tile1();
}

int main(void) 
{
    chan xscope_chan;
    par
    {
        xscope_host_data(xscope_chan);
        on tile[0]: main_tile0(xscope_chan);
        on tile[1]: main_tile1();
    }
    return 0;
}