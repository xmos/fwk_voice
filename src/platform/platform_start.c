// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>

#include "app_control/app_control.h"
#include "device_control_i2c.h"

#include "app_conf.h"
#include "usb_support.h"
#include "driver_instances.h"
#include "aic3204.h"

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
                         (rtos_i2c_slave_start_cb_t) i2c_dev_ctrl_start_cb,
                         (rtos_i2c_slave_rx_cb_t) i2c_dev_ctrl_rx_cb,
                         (rtos_i2c_slave_tx_start_cb_t) i2c_dev_ctrl_tx_start_cb,
                         (rtos_i2c_slave_tx_done_cb_t) NULL,
                         appconfI2C_INTERRUPT_CORE,
                         appconfI2C_TASK_PRIORITY);
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

static void i2s_start(void)
{
#if appconfI2S_ENABLED && ON_TILE(AUDIO_HW_TILE_NO)
    rtos_i2s_start(
            i2s_ctx,
            rtos_i2s_mclk_bclk_ratio(appconfAUDIO_CLOCK_FREQUENCY, appconfAUDIO_PIPELINE_SAMPLE_RATE),
            I2S_MODE_I2S,
            2.2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE,
            1.2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE,
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
    audio_codec_start();
    mics_start();
    i2s_start();
    usb_start();
}
