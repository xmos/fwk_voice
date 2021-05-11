// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <xcore/port.h>
#include <rtos_printf.h>
#include "app_pll_ctrl.h"

bool tud_xcore_sof_cb(uint8_t rhport)
{
    int valid;
    int last_acc;
    static int32_t acc;
    static uint16_t last_time;

    uint16_t cur_time = port_get_trigger_time(PORT_MCLK_IN);
    uint16_t diff_time = cur_time - last_time;
    last_time = cur_time;

    valid = diff_time >= 2918 && diff_time <= 3226;

    if (valid) {
        last_acc = acc;
        acc += 3072;
        acc -= diff_time;
    } else {
        last_acc = 0;
        acc = 0;
    }

#if APP_PLL_NUDGE_METHOD == 1
    /*
     * With method 1, nudges are temporary, so must nudge
     * repeatedly while the accumulator is above or below 0.
     */
    if (acc < 0) {
        app_pll_nudge(APP_PLL_NUDGE_SLOWER);
    } else if (acc > 0) {
        app_pll_nudge(APP_PLL_NUDGE_FASTER);
    }
#elif APP_PLL_NUDGE_METHOD == 2
    /*
     * With method 2, nudges are permanent, so must only nudge
     * if the accumulator is actively increasing or decreasing
     * in the wrong direction.
     */
    if (acc < 0 && acc < last_acc) {
        app_pll_nudge(APP_PLL_NUDGE_SLOWER);
    } else if (acc > 0 && acc > last_acc) {
        app_pll_nudge(APP_PLL_NUDGE_FASTER);
    }
#endif

//    rtos_printf("%u->%d\n", diff_time, acc);

    /* False tells TinyUSB to not send the SOF event to the stack */
    return false;
}
