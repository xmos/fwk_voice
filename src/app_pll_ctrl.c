// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <xcore/hwtimer.h>
#include "app_pll_ctrl.h"

void app_pll_nudge(int nudge)
{
    unsigned tileid = get_local_tile_id();

#if APP_PLL_NUDGE_METHOD == 1
    /*
     * This is similar to how 3510/3610 nudges the clock frequency.
     * Only one write of the slower/faster frequency though doesn't seem
     * to be very reliable. Doing it 3 times seems better. But this is not
     * scientific. I think method 2 is safer.
     */
    if (nudge > 0) {
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_FAST);
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_FAST);
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_FAST);
    } else if (nudge < 0) {
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_SLOW);
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_SLOW);
        write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_SLOW);
    }
    write_sswitch_reg_no_ack(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_NOM);

#elif APP_PLL_NUDGE_METHOD == 2
    /* This actually increments or decrements the frequency by a small step */
    static uint32_t f = (APP_PLL_FRAC_NOM & 0x0000FF00) >> 8;
    uint32_t fracval = APP_PLL_FRAC_NOM & 0xFFFF00FF;

    if (nudge > 0) {
        if (f < 255) {
            f++;
        }
    } else if (nudge < 0) {
        if (f > 0) {
            f--;
        }
    }

    fracval |= (f << 8);
    write_sswitch_reg_no_ack(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, fracval);

//    rtos_printf("%d\n", f);
#endif
}

void app_pll_init(void)
{
    unsigned tileid = get_local_tile_id();

    const unsigned APP_PLL_DISABLE = 0x0201FF04;
    const unsigned APP_PLL_DIV_0   = 0x80000004;

    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_DISABLE);

    hwtimer_t tmr = hwtimer_alloc();
    {
        xassert(tmr != 0);
        hwtimer_delay(tmr, 100000); // 1ms with 100 MHz timer tick
    }
    hwtimer_free(tmr);

    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_CTL_VAL);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_CTL_VAL);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_NOM);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_CLK_DIVIDER_NUM, APP_PLL_DIV_0);
}
