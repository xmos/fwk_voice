// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <xcore/channel.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "queue.h"

/* Library headers */
#include "rtos/drivers/gpio/api/rtos_gpio.h"
#include "rtos/drivers/i2c/api/rtos_i2c_master.h"
#include "rtos/drivers/i2s/api/rtos_i2s.h"
#include "rtos/drivers/intertile/api/rtos_intertile.h"
#include "rtos/drivers/mic_array/api/rtos_mic_array.h"
#include "rtos/drivers/qspi_flash/api/rtos_qspi_flash.h"
#include "rtos/drivers/spi/api/rtos_spi_master.h"

#include "rtos_printf.h"
#include "rtos_usb.h"
#include "device_control.h"

/* App headers */
#include "app_conf.h"
#include "board_init.h"
#include "usb_support.h"
#include "usb_audio.h"
#include "vfe_pipeline/vfe_pipeline.h"
#include "ww_model_runner/ww_model_runner.h"

#include "gpio_test/gpio_test.h"

typedef struct {
    rtos_mic_array_t *mic_array_ctx;
    rtos_i2s_t *i2s_ctx;
} audio_interfaces_t;

#define I2C_TILE 0
#define AUDIO_TILE 1
#define WW_TILE 0

#if ON_TILE(0)
static rtos_qspi_flash_t qspi_flash_ctx_s;
static rtos_qspi_flash_t *qspi_flash_ctx = &qspi_flash_ctx_s;
#endif

#if ON_TILE(I2C_TILE)
static rtos_i2c_master_t i2c_master_ctx_s;
static rtos_i2c_master_t *i2c_master_ctx = &i2c_master_ctx_s;
#endif

#if ON_TILE(AUDIO_TILE)
static rtos_mic_array_t mic_array_ctx_s;
static rtos_i2s_t i2s_ctx_s;

static rtos_mic_array_t *mic_array_ctx = &mic_array_ctx_s;
static rtos_i2s_t *i2s_ctx = &i2s_ctx_s;

static audio_interfaces_t audio_interfaces = {
        .mic_array_ctx = &mic_array_ctx_s,
        .i2s_ctx = &i2s_ctx_s,
};
#endif

#if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
static device_control_t device_control_usb_ctx_s;
static device_control_t *device_control_usb_ctx = (device_control_t *) &device_control_usb_ctx_s;
#endif

static rtos_gpio_t gpio_ctx_t0_s;
static rtos_gpio_t *gpio_ctx_t0 = &gpio_ctx_t0_s;

static rtos_gpio_t gpio_ctx_t1_s;
static rtos_gpio_t *gpio_ctx_t1 = &gpio_ctx_t1_s;

static rtos_intertile_t intertile_ctx_s;
static rtos_intertile_t *intertile_ctx = &intertile_ctx_s;

chanend_t other_tile_c;

volatile int mic_from_usb = 0;
volatile int aec_ref_source = appconfAEC_REF_DEFAULT;

void vfe_pipeline_input(void *input_app_data,
                        int32_t (*mic_audio_frame)[2],
                        int32_t (*ref_audio_frame)[2],
                        size_t frame_count)
{
    audio_interfaces_t *audio_interfaces = input_app_data;

    static int good;
    if (!good) {
        for (int i = 0; i < 10; i++) {
            rtos_mic_array_rx(audio_interfaces->mic_array_ctx,
                              mic_audio_frame,
                              frame_count,
                              portMAX_DELAY);
        }
        good = 1;
    }

    /*
     * NOTE: ALWAYS receive the next frame from the PDM mics,
     * even if USB is the current mic source. The controls the
     * timing since usb_audio_recv() does not block and will
     * receive all zeros if no frame is available yet.
     */
    rtos_mic_array_rx(audio_interfaces->mic_array_ctx,
                      mic_audio_frame,
                      frame_count,
                      portMAX_DELAY);

#if appconfUSB_ENABLED
    int32_t (*usb_mic_audio_frame)[2] = NULL;

    if (mic_from_usb) {
        usb_mic_audio_frame = mic_audio_frame;
    }

    /*
     * As noted above, this does not block.
     */
    usb_audio_recv(intertile_ctx,
                   frame_count,
                   ref_audio_frame,
                   usb_mic_audio_frame);
#endif

#if appconfI2S_ENABLED
    if (!appconfUSB_ENABLED || aec_ref_source == appconfAEC_REF_I2S) {
        /* This shouldn't need to block given it shares a clock with the PDM mics */

        size_t rx_count =
        rtos_i2s_rx(audio_interfaces->i2s_ctx,
                    (int32_t*) ref_audio_frame,
                    frame_count,
                    portMAX_DELAY);
        xassert(rx_count == frame_count);
    }
#endif
}

int vfe_pipeline_output(void *output_app_data,
                        int32_t (*proc_audio_frame)[2],
                        int32_t (*mic_audio_frame)[2],
                        int32_t (*ref_audio_frame)[2],
                        size_t frame_count)
{
#if appconfI2S_ENABLED
    rtos_i2s_t *i2s_ctx = output_app_data;

    rtos_i2s_tx(i2s_ctx,
                (int32_t*) proc_audio_frame,
                frame_count,
                portMAX_DELAY);
#endif

#if appconfUSB_ENABLED
    usb_audio_send(intertile_ctx,
                  frame_count,
                  proc_audio_frame,
                  mic_audio_frame,
                  ref_audio_frame);
#endif

    return VFE_PIPELINE_FREE_FRAME;
}

void usb_device_control_set_ctx(device_control_t *ctx,
                                size_t servicer_count);

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    for(;;);
}

#define DEVICE_CONTROL_USB_PORT 1
#define DEVICE_CONTROL_USB_CLIENT_PRIORITY  (configMAX_PRIORITIES-1)

DEVICE_CONTROL_CALLBACK_ATTR
control_ret_t read_cmd(control_resid_t resid, control_cmd_t cmd, uint8_t *payload, size_t payload_len, void *app_data)
{
    rtos_printf("Device control READ\n\t");

    rtos_printf("Servicer on tile %d received command %02x for resid %02x\n\t", THIS_XCORE_TILE, cmd, resid);
    rtos_printf("The command is requesting %d bytes\n\t", payload_len);
    for (int i = 0; i < payload_len; i++) {
        payload[i] = (cmd & 0x7F) + i;
    }
    rtos_printf("Bytes to be sent are:\n\t", payload_len);
    for (int i = 0; i < payload_len; i++) {
        rtos_printf("%02x ", payload[i]);
    }
    rtos_printf("\n\n");

    return CONTROL_SUCCESS;
}

DEVICE_CONTROL_CALLBACK_ATTR
control_ret_t write_cmd(control_resid_t resid, control_cmd_t cmd, const uint8_t *payload, size_t payload_len, void *app_data)
{
    rtos_printf("Device control WRITE\n\t");

    rtos_printf("Servicer on tile %d received command %02x for resid %02x\n\t", THIS_XCORE_TILE, cmd, resid);
    rtos_printf("The command has %d bytes\n\t", payload_len);
    rtos_printf("Bytes received are:\n\t", payload_len);
    for (int i = 0; i < payload_len; i++) {
        rtos_printf("%02x ", payload[i]);
    }
    rtos_printf("\n\n");

    return CONTROL_SUCCESS;
}

static void mem_analysis( void *arg )
{
	for( ;; ) {
		rtos_printf("Tile[%d]:\n\tMinimum heap free: %d\n\tCurrent heap free: %d\n", THIS_XCORE_TILE, xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());
		vTaskDelay( pdMS_TO_TICKS( 5000 ) );
	}
}

void startup_task(void *arg)
{
    xTaskCreate( mem_analysis, "mem_an", portTASK_STACK_DEPTH(mem_analysis), NULL, configMAX_PRIORITIES, NULL );

    uint32_t dac_configured;
    #if ON_TILE(USB_TILE_NO) /* TODO: both tiles once I2C dev control is added */
    control_ret_t dc_ret;
    device_control_t *device_control_ctxs[] = {
#if appconfUSB_ENABLED
            device_control_usb_ctx,
#endif
            /* TODO: Add I2C dev control context */
    };
    #endif

    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

    rtos_intertile_start(intertile_ctx);

    #if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    {
        dc_ret = device_control_start(device_control_usb_ctx,
                                      appconfDEVICE_CONTROL_PORT,
                                      appconfDEVICE_CONTROL_TASK_PRIORITY);
        xassert(dc_ret == CONTROL_SUCCESS);
    }
    #endif

#if appconfI2S_ENABLED
    #if ON_TILE(I2C_TILE)
    {
        int dac_init(rtos_i2c_master_t *i2c_ctx);

        rtos_i2c_master_start(i2c_master_ctx);

        if (dac_init(i2c_master_ctx) == 0) {
            dac_configured = 1;
        } else {
            rtos_printf("DAC initialization failed\n");
            dac_configured = 0;
        }
        chan_out_byte(other_tile_c, dac_configured);
    }
    #else
    {
        dac_configured = chan_in_byte(other_tile_c);
    }
    #endif

    if (!dac_configured) {
        vTaskDelete(NULL);
    }
#endif

    chanend_free(other_tile_c);

    rtos_gpio_rpc_config(gpio_ctx_t0, appconfGPIO_T0_RPC_PORT, appconfGPIO_RPC_HOST_PRIORITY);
    rtos_gpio_rpc_config(gpio_ctx_t1, appconfGPIO_T1_RPC_PORT, appconfGPIO_RPC_HOST_PRIORITY);
    #if ON_TILE(0)
    {
        rtos_qspi_flash_start(qspi_flash_ctx, appconfQSPI_FLASH_TASK_PRIORITY);
        rtos_gpio_start(gpio_ctx_t0);
    }
    #endif

    #if ON_TILE(1)
    {
        rtos_gpio_start(gpio_ctx_t1);

        gpio_test(gpio_ctx_t0);
    }
    #endif

    #if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    {
        usb_audio_init(intertile_ctx, appconfUSB_AUDIO_TASK_PRIORITY);
        usb_device_control_set_ctx(device_control_usb_ctx, 1);
        usb_manager_start(appconfUSB_MGR_TASK_PRIORITY);
    }
    #endif

    #if ON_TILE(AUDIO_TILE)
    {
        const int pdm_decimation_factor = rtos_mic_array_decimation_factor(
                PDM_CLOCK_FREQUENCY,
                VFE_PIPELINE_AUDIO_SAMPLE_RATE);

        rtos_mic_array_start(
                mic_array_ctx,
                pdm_decimation_factor,
                rtos_mic_array_third_stage_coefs(pdm_decimation_factor),
                rtos_mic_array_fir_compensation(pdm_decimation_factor),
                2 * MIC_DUAL_FRAME_SIZE,
                appconfPDM_MIC_INTERRUPT_CORE);

#if appconfI2S_ENABLED
        rtos_i2s_start(
                i2s_ctx,
                rtos_i2s_mclk_bclk_ratio(AUDIO_CLOCK_FREQUENCY, VFE_PIPELINE_AUDIO_SAMPLE_RATE),
                I2S_MODE_I2S,
                2.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                1.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                appconfI2S_INTERRUPT_CORE);
#endif

        vfe_pipeline_init(&audio_interfaces, audio_interfaces.i2s_ctx);
    }
    #endif

    #if ON_TILE(WW_TILE)
    {
#if WW
        StreamBufferHandle_t audio_stream = xStreamBufferCreate(
                                                   1.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                                                   appconfWW_FRAMES_PER_INFERENCE);
        /* TODO Task that fills audio_stream with uint16_t samples */
        ww_task_create(audio_stream, appconfWW_TASK_PRIORITY);
#endif
    }
    #endif

    #if ON_TILE(USB_TILE_NO)
    {
        control_resid_t resources[] = {0x3, 0x6, 0x9};

        device_control_servicer_t servicer_ctx;
        rtos_printf("Will register a servicer now on tile %d\n", THIS_XCORE_TILE);
        dc_ret = device_control_servicer_register(&servicer_ctx,
                                                  device_control_ctxs,
                                                  sizeof(device_control_ctxs) / sizeof(device_control_ctxs[0]),
                                                  resources,
                                                  sizeof(resources));
        xassert(dc_ret == CONTROL_SUCCESS);
        rtos_printf("Servicer registered now on tile %d\n", THIS_XCORE_TILE);

        for (;;) {
            device_control_servicer_cmd_recv(&servicer_ctx, read_cmd, write_cmd, NULL, RTOS_OSAL_WAIT_FOREVER);
        }
    }
    #endif

    vTaskDelete(NULL);
}

void vApplicationIdleHook(void)
{
    asm volatile("waiteu");
}

#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    board_tile0_init(c1, intertile_ctx, i2c_master_ctx, gpio_ctx_t0, gpio_ctx_t1, qspi_flash_ctx);
    (void) c2;
    (void) c3;

    other_tile_c = c1;

#if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    usb_manager_init();

    device_control_init(device_control_usb_ctx,
                        THIS_XCORE_TILE == USB_TILE_NO ? DEVICE_CONTROL_HOST_MODE : DEVICE_CONTROL_CLIENT_MODE,
                        NULL,
                        0);
#endif

    xTaskCreate((TaskFunction_t) startup_task,
                "startup_task",
                RTOS_THREAD_STACK_SIZE(startup_task),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    rtos_printf("start scheduler on tile 0\n");
    vTaskStartScheduler();
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    board_tile1_init(c0, intertile_ctx, mic_array_ctx, i2s_ctx, gpio_ctx_t0, gpio_ctx_t1);
    (void) c1;
    (void) c2;
    (void) c3;

    other_tile_c = c0;

#if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    usb_manager_init();
#endif

    xTaskCreate((TaskFunction_t) startup_task,
                "startup_task",
                RTOS_THREAD_STACK_SIZE(startup_task),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    rtos_printf("start scheduler on tile 1\n");
    vTaskStartScheduler();
}
#endif
