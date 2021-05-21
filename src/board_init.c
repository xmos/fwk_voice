// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <stdarg.h>
#include <xcore/hwtimer.h>

#include "app_pll_ctrl.h"
#include "board_init.h"

#include "app_conf.h"

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

static rtos_driver_rpc_t gpio_rpc_config_t0;
static rtos_driver_rpc_t gpio_rpc_config_t1;

void board_tile0_init(
        chanend_t tile1,
        rtos_intertile_t *intertile_ctx,
        rtos_i2c_master_t *i2c_master_ctx,
        rtos_gpio_t *gpio_ctx_t0,
        rtos_gpio_t *gpio_ctx_t1,
        rtos_qspi_flash_t *qspi_flash_ctx)
{
    rtos_intertile_t *client_intertile_ctx[1] = {intertile_ctx};
    rtos_intertile_init(intertile_ctx, tile1);

    /*
     * Configure the MCLK input port on tile 0.
     * This is wired to app PLL/MCLK output from tile 1.
     * It is set up to clock itself. This allows GETTS to
     * be called on it to count its clock cycles. This
     * count is used to nudge its frequency to match the
     * USB host.
     */
    port_init(PORT_MCLK_IN, PORT_INPUT, PORT_UNBUFFERED);
    clock_enable(XS1_CLKBLK_4);
    clock_set_source_port(XS1_CLKBLK_4, PORT_MCLK_IN);
    port_set_clock(PORT_MCLK_IN, XS1_CLKBLK_4);
    clock_start(XS1_CLKBLK_4);

    rtos_gpio_init(gpio_ctx_t0);

    rtos_qspi_flash_init(
            qspi_flash_ctx,
            XS1_CLKBLK_1,
            PORT_SQI_CS,
            PORT_SQI_SCLK,
            PORT_SQI_SIO,

            /** Derive QSPI clock from the 600 MHz xcore clock **/
            qspi_io_source_clock_xcore,

            /** Full speed clock configuration **/
            5, // 600 MHz / (2*5) -> 60 MHz,
            1,
            qspi_io_sample_edge_rising,
            0,

            /** SPI read clock configuration **/
            12, // 600 MHz / (2*12) -> 25 MHz
            0,
            qspi_io_sample_edge_falling,
            0,

            qspi_flash_page_program_1_4_4);

    rtos_i2c_master_init(
            i2c_master_ctx,
            PORT_I2C_SCL, 0, 0,
            PORT_I2C_SDA, 0, 0,
            0,
            100);

    rtos_gpio_rpc_host_init(
            gpio_ctx_t0,
            &gpio_rpc_config_t0,
            client_intertile_ctx,
            1);

    rtos_gpio_rpc_client_init(
            gpio_ctx_t1,
            &gpio_rpc_config_t1,
            intertile_ctx);
}

void board_tile1_init(
        chanend_t tile0,
        rtos_intertile_t *intertile_ctx,
        rtos_mic_array_t *mic_array_ctx,
        rtos_i2s_t *i2s_ctx,
        rtos_gpio_t *gpio_ctx_t0,
        rtos_gpio_t *gpio_ctx_t1)
{
    rtos_intertile_t *client_intertile_ctx[1] = {intertile_ctx};
    rtos_intertile_init(intertile_ctx, tile0);

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
    port_t p_i2s_din[1] = {
            PORT_I2S_ADC_DATA
    };
    port_t p_bclk = PORT_I2S_BCLK;
    port_t p_lrclk = PORT_I2S_LRCLK;

    /* Clock blocks for I2S */
    xclock_t bclk = clock_init(XS1_CLKBLK_3);

    app_pll_init();

    rtos_gpio_init(gpio_ctx_t1);

    rtos_mic_array_init(
            mic_array_ctx,
            (1 << appconfPDM_MIC_IO_CORE),
            pdmclk,
            pdmclk2,
            AUDIO_CLOCK_FREQUENCY / PDM_CLOCK_FREQUENCY,
            p_mclk,
            p_pdm_clk,
            p_pdm_mics);

#if appconfI2S_ENABLED
#if appconfI2S_MODE == appconfI2S_MODE_MASTER
    rtos_i2s_master_init(
            i2s_ctx,
            (1 << appconfI2S_IO_CORE),
            p_i2s_dout,
            1,
            p_i2s_din,
            1,
            p_bclk,
            p_lrclk,
            p_mclk,
            bclk);
#elif appconfI2S_MODE == appconfI2S_MODE_SLAVE
    rtos_i2s_slave_init(
            i2s_ctx,
            (1 << appconfI2S_IO_CORE),
            p_i2s_dout,
            1,
            p_i2s_din,
            1,
            p_bclk,
            p_lrclk,
            bclk);
#else
#error Invalid I2S mode
#endif

#else

    (void) i2s_ctx;

#endif

    rtos_gpio_rpc_client_init(
            gpio_ctx_t0,
            &gpio_rpc_config_t0,
            intertile_ctx);

    rtos_gpio_rpc_host_init(
            gpio_ctx_t1,
            &gpio_rpc_config_t1,
            client_intertile_ctx,
            1);
}
