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
void main_tile0(chanend, chanend);
void main_tile1(chanend);
}
void burn_div() {
    unsafe {
    while(1) {
        float_s32_t a, b, c;
        a = double_to_float_s32(5.678765);
        b = double_to_float_s32(3.5667);
        volatile float_s32_t * unsafe p = &c;
        for(int i=0; i<32; i++) {
            *p = float_s32_div(b, a);
            b = *p;
        }
    }
    }
}

int main (void)
{
  chan c_cross_tile, xscope_chan;
  par
  {
#if TEST_WAV_XSCOPE
    xscope_host_data(xscope_chan);
#endif
    on tile[0]: {
        par {
            par(int t=0; t<8-1; t++) {
                {
                    set_core_fast_mode_on();
                    //../../../../../audio_test_tools/audio_test_tools/src/burners.S:(.text+0x84): Error: byte offset 0xffffff7e for relocation R_XCORE1_REL16_4 is misaligned
                    //../../../../../audio_test_tools/audio_test_tools/src/burners.S: Error: relocation with respect to 'div_loop'
                    //att_burn_thread_div();
                    //while(1) {continue;}
                    burn_div();
                }
            }
            {
            main_tile0(c_cross_tile, xscope_chan);
            _Exit(0);
            }
        }
    }
    on tile[1]: main_tile1(c_cross_tile);
  }
  return 0;
}
