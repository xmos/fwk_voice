// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>

#include "app_control/app_control.h"
#include "device_control_i2c.h"

#include "app_conf.h"
#include "usb_support.h"
#include "driver_instances.h"
#include "aic3204.h"
#include "xcore_device_memory.h"

static void gpio_start(void)
{
    rtos_gpio_rpc_config(gpio_ctx_t0, appconfGPIO_T0_RPC_PORT, appconfGPIO_RPC_HOST_PRIORITY);
    rtos_gpio_rpc_config(gpio_ctx_t1, appconfGPIO_T1_RPC_PORT, appconfGPIO_RPC_HOST_PRIORITY);

#if ON_TILE(0)
    rtos_gpio_start(gpio_ctx_t0);
#endif
#if ON_TILE(1)
    rtos_gpio_start(gpio_ctx_t1);
#endif
}

static void flash_start(void)
{
#if ON_TILE(FLASH_TILE_NO)
    rtos_qspi_flash_start(qspi_flash_ctx, appconfQSPI_FLASH_TASK_PRIORITY);
    #if (USE_SWMEM == 1)

    /*
     * TODO: Move the interrupt_core parameter into the swmem driver
     */
    uint32_t core_exclude_map;

    /* Ensure that the mic array interrupt is enabled on the requested core */
    rtos_osal_thread_core_exclusion_get(NULL, &core_exclude_map);
    rtos_osal_thread_core_exclusion_set(NULL, ~(1 << appconfSWMEM_INTERRUPT_CORE));

    swmem_setup(qspi_flash_ctx, 0 /*ignored*/);

    /* Restore the core exclusion map for the calling thread */
    rtos_osal_thread_core_exclusion_set(NULL, core_exclude_map);

    #endif
#endif
}

static void i2c_start(void)
{
#if appconfI2S_ENABLED && ON_TILE(I2C_TILE_NO)
    rtos_i2c_master_start(i2c_master_ctx);
#endif

#if appconfI2C_CTRL_ENABLED && ON_TILE(I2C_CTRL_TILE_NO)
    rtos_i2c_slave_start(i2c_slave_ctx,
                         device_control_i2c_ctx,
                         (rtos_i2c_slave_start_cb_t) device_control_i2c_start_cb,
                         (rtos_i2c_slave_rx_cb_t) device_control_i2c_rx_cb,
                         (rtos_i2c_slave_tx_start_cb_t) device_control_i2c_tx_start_cb,
                         (rtos_i2c_slave_tx_done_cb_t) NULL,
                         appconfI2C_INTERRUPT_CORE,
                         appconfI2C_TASK_PRIORITY);
#endif
}

#if 0
RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_start_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    static uint8_t tx_buf[32];
    static uint8_t rx_buf[32];

    rtos_printf("SPI SLAVE STARTING!\n");

    for (int i = 0; i < 32; i++) {
        tx_buf[i] = i | 0x80;
    }

    spi_slave_xfer_prepare(ctx, rx_buf, sizeof(rx_buf), tx_buf, sizeof(tx_buf));
}

RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_xfer_done_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    size_t rx_len;
    size_t tx_len;

    if (spi_slave_xfer_complete(ctx, &rx_buf, &rx_len, &tx_buf, &tx_len, 0) == 0) {
        rtos_printf("SPI slave xfer complete\n");
        rtos_printf("%d bytes sent, %d bytes received\n", tx_len, rx_len);
        rtos_printf("TX: ");
        for (int i = 0; i < 32; i++) {
            rtos_printf("%02x ", tx_buf[i]);
        }
        rtos_printf("\n");
        rtos_printf("RX: ");
        for (int i = 0; i < 32; i++) {
            rtos_printf("%02x ", rx_buf[i]);
        }
        rtos_printf("\n");
    }
}
#endif

void spi_slave_start_cb(rtos_spi_slave_t *ctx, void *app_data);
void spi_slave_xfer_done_cb(rtos_spi_slave_t *ctx, void *app_data);

static void spi_start(void)
{
#if appconfSPI_OUTPUT_ENABLED && ON_TILE(SPI_OUTPUT_TILE_NO)

    const rtos_gpio_port_id_t wifi_rst_port = rtos_gpio_port(WIFI_WUP_RST_N);
    rtos_gpio_port_enable(gpio_ctx_t0, wifi_rst_port);
    rtos_gpio_port_out(gpio_ctx_t0, wifi_rst_port, 0x00);

    const rtos_gpio_port_id_t wifi_cs_port = rtos_gpio_port(WIFI_CS_N);
    rtos_gpio_port_enable(gpio_ctx_t0, wifi_cs_port);
    rtos_gpio_port_out(gpio_ctx_t0, wifi_cs_port, 0x0F);

    rtos_spi_slave_start(spi_slave_ctx,
                         NULL,
                         (rtos_spi_slave_start_cb_t) spi_slave_start_cb,
                         (rtos_spi_slave_xfer_done_cb_t) spi_slave_xfer_done_cb,
                         appconfSPI_INTERRUPT_CORE,
                         appconfSPI_TASK_PRIORITY);
#endif
}

static void audio_codec_start(void)
{
#if appconfI2S_ENABLED && ON_TILE(I2C_TILE_NO)
    if (aic3204_init() != 0) {
        rtos_printf("DAC initialization failed\n");
    }
#endif
}

static void mics_start(void)
{
#if ON_TILE(AUDIO_HW_TILE_NO)
    const int pdm_decimation_factor = rtos_mic_array_decimation_factor(
            appconfPDM_CLOCK_FREQUENCY,
            appconfAUDIO_PIPELINE_SAMPLE_RATE);

    rtos_mic_array_start(
            mic_array_ctx,
            pdm_decimation_factor,
            rtos_mic_array_third_stage_coefs(pdm_decimation_factor),
            rtos_mic_array_fir_compensation(pdm_decimation_factor),
            2 * MIC_DUAL_FRAME_SIZE,
            appconfPDM_MIC_INTERRUPT_CORE);
#endif
}

void i2s_rate_conversion_enable(void);

static void i2s_start(void)
{
#if appconfI2S_ENABLED && ON_TILE(AUDIO_HW_TILE_NO)

    if (appconfI2S_AUDIO_SAMPLE_RATE == 3*appconfAUDIO_PIPELINE_SAMPLE_RATE) {
        i2s_rate_conversion_enable();
    }

    rtos_i2s_start(
            i2s_ctx,
            rtos_i2s_mclk_bclk_ratio(appconfAUDIO_CLOCK_FREQUENCY, appconfI2S_AUDIO_SAMPLE_RATE),
            I2S_MODE_I2S,
            2.2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE,
            1.2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE * (appconfI2S_TDM_ENABLED ? 3 : 1),
            appconfI2S_INTERRUPT_CORE);
#endif
}

static void usb_start(void)
{
#if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    usb_manager_start(appconfUSB_MGR_TASK_PRIORITY);
#endif
}

void platform_start(void)
{
    rtos_intertile_start(intertile_ctx);

    gpio_start();
    flash_start();
    i2c_start();
    spi_start();
    audio_codec_start();
    mics_start();
    i2s_start();
    usb_start();
}
