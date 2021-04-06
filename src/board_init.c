// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <stdarg.h>
#include <xcore/hwtimer.h>

#include "board_init.h"

typedef enum {
    PORT_INPUT = 0,
    PORT_OUTPUT = 1,
    PORT_INOUT = 2,
} port_direction_t;

static port_t port_init(port_t p, port_direction_t d, port_type_t t, ...)
{
    if (t == PORT_UNBUFFERED) {
        port_enable(p);
    } else {
        size_t tw;
        va_list ap;
        va_start(ap, t);
        tw = va_arg(ap, size_t);
        va_end(ap);
        port_start_buffered(p, tw);
    }

    if (d == PORT_OUTPUT) {
        /* ensure port is in output mode. */
        port_out(p, 0);
    }

    return p;
}

static xclock_t clock_init(xclock_t c)
{
    clock_enable(c);

    return c;
}

static void set_app_pll(void)
{
    unsigned tileid = get_local_tile_id();

    xassert(AUDIO_CLOCK_FREQUENCY == 24576000);

    // 24MHz in, 24.576MHz out, integer mode
    // Found exact solution:   IN  24000000.0, OUT  24576000.0, VCO 2457600000.0, RD  5, FD  512                       , OD  5, FOD   10
    const unsigned APP_PLL_DISABLE = 0x0201FF04;
    const unsigned APP_PLL_CTL_0   = 0x0A01FF04;
    const unsigned APP_PLL_DIV_0   = 0x80000004;
    const unsigned APP_PLL_FRAC_0  = 0x00000000;

    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_DISABLE);

    hwtimer_t tmr = hwtimer_alloc();
    {
        xassert(tmr != 0);
        hwtimer_delay(tmr, 100000); // 1ms with 100 MHz timer tick
    }
    hwtimer_free(tmr);

    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_CTL_0);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_CTL_NUM, APP_PLL_CTL_0);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_PLL_FRAC_N_DIVIDER_NUM, APP_PLL_FRAC_0);
    write_sswitch_reg(tileid, XS1_SSWITCH_SS_APP_CLK_DIVIDER_NUM, APP_PLL_DIV_0);
}

void board_tile0_init(
        chanend_t tile1,
        rtos_intertile_t *intertile_ctx,
        rtos_i2c_master_t *i2c_master_ctx)
{
    rtos_intertile_init(intertile_ctx, tile1);

    rtos_i2c_master_init(
            i2c_master_ctx,
            PORT_I2C_SCL, 0, 0,
            PORT_I2C_SDA, 0, 0,
            0,
            100);
}

void board_tile1_init(
        chanend_t tile0,
        rtos_intertile_t *intertile_ctx,
        rtos_mic_array_t *mic_array_ctx,
        rtos_i2s_t *i2s_ctx)
{
    port_t p_rst_shared = port_init(PORT_CODEC_RST_N, PORT_OUTPUT, PORT_UNBUFFERED);
    port_out(p_rst_shared, 0xF);

    /* Clock blocks for PDM mics */
    xclock_t pdmclk = clock_init(XS1_CLKBLK_1);
    xclock_t pdmclk2 = clock_init(XS1_CLKBLK_2);

    /* Clock port for the PDM mics and I2S */
    port_t p_mclk = port_init(PORT_MCLK_IN, PORT_INPUT, PORT_UNBUFFERED);

    /* Ports for the PDM microphones */
    port_t p_pdm_clk = port_init(PORT_PDM_CLK,   PORT_OUTPUT, PORT_UNBUFFERED);
    port_t p_pdm_mics = port_init(PORT_PDM_DATA, PORT_INPUT,  PORT_BUFFERED, 32);

    /* Ports for the I2S. */
    port_t p_i2s_dout[1] = {
            PORT_I2S_DAC_DATA
    };
    port_t p_bclk = PORT_I2S_BCLK;
    port_t p_lrclk = PORT_I2S_LRCLK;

    /* Clock blocks for I2S */
    xclock_t bclk = clock_init(XS1_CLKBLK_3);

    set_app_pll();

    rtos_intertile_init(intertile_ctx, tile0);

    rtos_mic_array_init(
            mic_array_ctx,
            pdmclk,
            pdmclk2,
            AUDIO_CLOCK_FREQUENCY / PDM_CLOCK_FREQUENCY,
            p_mclk,
            p_pdm_clk,
            p_pdm_mics);

    rtos_i2s_master_init(
            i2s_ctx,
            p_i2s_dout,
            1,
            NULL,
            0,
            p_bclk,
            p_lrclk,
            p_mclk,
            bclk);
}
