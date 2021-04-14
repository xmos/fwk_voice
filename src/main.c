// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "queue.h"

#include "rtos_printf.h"

#include <xcore/channel.h>

#include "rtos_usb.h"
#include "usb_support.h"

#include "vfe_pipeline/vfe_pipeline.h"

#include "board_init.h"

#include "usb_audio.h"

#define I2C_TILE 0
#define AUDIO_TILE 1


#if ON_TILE(I2C_TILE)
static rtos_i2c_master_t i2c_master_ctx_s;
static rtos_i2c_master_t *i2c_master_ctx = &i2c_master_ctx_s;
#endif

#if ON_TILE(AUDIO_TILE)
static rtos_mic_array_t mic_array_ctx_s;
static rtos_i2s_t i2s_ctx_s;

static rtos_mic_array_t *mic_array_ctx = &mic_array_ctx_s;
static rtos_i2s_t *i2s_ctx = &i2s_ctx_s;
#endif

static rtos_intertile_t intertile_ctx_s;
static rtos_intertile_t *intertile_ctx = &intertile_ctx_s;

chanend_t other_tile_c;

void vfe_pipeline_input(void *mic_array_ctx,
                        int32_t (*audio_frame)[2],
                        size_t frame_count)
{
    rtos_mic_array_rx((rtos_mic_array_t *) mic_array_ctx,
                      audio_frame,
                      frame_count,
                      portMAX_DELAY);
}

int vfe_pipeline_output(void *i2s_ctx,
                         int32_t (*audio_frame)[2],
                         size_t frame_count)
{
    rtos_i2s_tx((rtos_i2s_t *) i2s_ctx,
                       (int32_t*) audio_frame,
                       frame_count,
                       portMAX_DELAY);

    rtos_intertile_tx(intertile_ctx,
                      0,
                      audio_frame,
                      frame_count * 2 * sizeof(int32_t));

    return VFE_PIPELINE_FREE_FRAME;
}

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    for(;;);
}

void vApplicationDaemonTaskStartup(void *arg)
{
    uint32_t dac_configured;

    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

    rtos_intertile_start(intertile_ctx);

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

    chanend_free(other_tile_c);

    if (!dac_configured) {
        vTaskDelete(NULL);
    }

    #if ON_TILE(USB_TILE_NO)
    {
        usb_audio_init(intertile_ctx, configMAX_PRIORITIES-1);
        usb_manager_start(configMAX_PRIORITIES-1);
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
                3);

        rtos_i2s_start(
                i2s_ctx,
                rtos_i2s_mclk_bclk_ratio(AUDIO_CLOCK_FREQUENCY, VFE_PIPELINE_AUDIO_SAMPLE_RATE),
                I2S_MODE_I2S,
                0,
                1.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                4);

        vfe_pipeline_init(mic_array_ctx, i2s_ctx);
    }
    #endif

    vTaskDelete(NULL);
}


#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    board_tile0_init(c1, intertile_ctx, i2c_master_ctx);
    (void) c2;
    (void) c3;

    other_tile_c = c1;

#if ON_TILE(USB_TILE_NO)
    usb_manager_init();
#endif

    xTaskCreate((TaskFunction_t) vApplicationDaemonTaskStartup,
                "vApplicationDaemonTaskStartup",
                RTOS_THREAD_STACK_SIZE(vApplicationDaemonTaskStartup),
                NULL,
                1,
                NULL);

    rtos_printf("start scheduler on tile 0\n");
    vTaskStartScheduler();
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    board_tile1_init(c0, intertile_ctx, mic_array_ctx, i2s_ctx);
    (void) c1;
    (void) c2;
    (void) c3;

    other_tile_c = c0;

#if ON_TILE(USB_TILE_NO)
    usb_manager_init();
#endif

    xTaskCreate((TaskFunction_t) vApplicationDaemonTaskStartup,
                "vApplicationDaemonTaskStartup",
                RTOS_THREAD_STACK_SIZE(vApplicationDaemonTaskStartup),
                NULL,
                configMAX_PRIORITIES-1,
                NULL);

    rtos_printf("start scheduler on tile 1\n");
    vTaskStartScheduler();
}
#endif
