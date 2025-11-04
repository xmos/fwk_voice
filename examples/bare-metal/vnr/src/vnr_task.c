// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <xcore/channel.h>
#include <xcore/chanend.h>

#include "vnr.h"

#if PROFILE_PROCESSING
#include "profile.h"
#else
static void prof(int n, const char* str) {}
static void print_prof(int a, int b, int framenum){}
#endif

void vnr(chanend_t c_frame_in, chanend_t c_frame_out)
{
    vnr_ctx_t vnr_ctx;
    prof(0, "start_vnr_init");
    vnr_init(&vnr_ctx);
    prof(1, "end_vnr_init");

    int framenum = 0;
    while(1)
    {
        prof(2, "receive_frame");
        int32_t new_frame[240];
        chan_in_buf_word(c_frame_in, (uint32_t*)&new_frame[0], 240);

        prof(3, "add frame");
        vnr_add_frame(&vnr_ctx, &new_frame[0]);
        
        prof(4, "vnr_compute start");
        vnr_compute(&vnr_ctx);

        prof(5, "send output");
        float_s32_t inference_output = vnr_ctx.vnr_output_s32;
        chan_out_buf_byte(c_frame_out, (uint8_t*)&inference_output, sizeof(float_s32_t));
        
        framenum++;
        print_prof(0, 6, framenum);        
    }
}
