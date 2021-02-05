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

#include "vfe_pipeline/vfe_pipeline.h"

#include "board_init.h"

#define I2C_TILE 0
#define AUDIO_TILE 1


#if ON_TILE(I2C_TILE)
static rtos_i2c_master_t i2c_master_ctx_s;
static rtos_i2c_master_t *i2c_master_ctx = &i2c_master_ctx_s;
#endif

#if ON_TILE(AUDIO_TILE)
static rtos_mic_array_t mic_array_ctx_s;
static rtos_i2s_master_t i2s_master_ctx_s;

static rtos_mic_array_t *mic_array_ctx = &mic_array_ctx_s;
static rtos_i2s_master_t *i2s_master_ctx = &i2s_master_ctx_s;
#endif

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

void vfe_pipeline_output(void *i2s_master_ctx,
                         int32_t (*audio_frame)[2],
                         size_t frame_count)
{
    rtos_i2s_master_tx((rtos_i2s_master_t *) i2s_master_ctx,
                       (int32_t*) audio_frame,
                       frame_count,
                       portMAX_DELAY);
}

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    for(;;);
}

void vApplicationCoreInitHook(BaseType_t xCoreID)
{
#if ON_TILE(0)
    rtos_printf("Initializing tile 0, core %d on core %d\n", xCoreID, portGET_CORE_ID());
#endif

#if ON_TILE(1)
    rtos_printf("Initializing tile 1 core %d on core %d\n", xCoreID, portGET_CORE_ID());

    switch (xCoreID) {

    case 0:
        rtos_mic_array_interrupt_init(mic_array_ctx);
        break;
    case 1:
        rtos_i2s_master_interrupt_init(i2s_master_ctx);
        break;
    }

#endif
}

void vApplicationDaemonTaskStartup(void *arg)
{
    uint32_t dac_configured;

    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

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
                configMAX_PRIORITIES-1);

        rtos_i2s_master_start(
                i2s_master_ctx,
                rtos_i2s_master_mclk_bclk_ratio(AUDIO_CLOCK_FREQUENCY, VFE_PIPELINE_AUDIO_SAMPLE_RATE),
                I2S_MODE_I2S,
                1.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                configMAX_PRIORITIES-1);

        vfe_pipeline_init(mic_array_ctx, i2s_master_ctx);
    }
    #endif

    vTaskDelete(NULL);
}


#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    board_tile0_init(c1, i2c_master_ctx);
    (void) c2;
    (void) c3;

    other_tile_c = c1;

    xTaskCreate((TaskFunction_t) vApplicationDaemonTaskStartup,
                "vApplicationDaemonTaskStartup",
                RTOS_THREAD_STACK_SIZE(vApplicationDaemonTaskStartup),
                NULL,
                1,
                NULL);

    rtos_printf("start scheduler on tile 0\n");
    vTaskStartScheduler();

    return;
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    board_tile1_init(c0, mic_array_ctx, i2s_master_ctx);
    (void) c1;
    (void) c2;
    (void) c3;

    other_tile_c = c0;

    xTaskCreate((TaskFunction_t) vApplicationDaemonTaskStartup,
                "vApplicationDaemonTaskStartup",
                RTOS_THREAD_STACK_SIZE(vApplicationDaemonTaskStartup),
                NULL,
                configMAX_PRIORITIES-1,
                NULL);

    rtos_printf("start scheduler on tile 1\n");
    vTaskStartScheduler();

    return;
}
#endif
