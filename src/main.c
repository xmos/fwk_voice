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
#include "rtos_printf.h"
#include "device_control.h"
#include "src.h"

/* App headers */
#include "app_conf.h"
#include "platform/platform_init.h"
#include "platform/driver_instances.h"
#include "app_control/app_control.h"
#include "usb_support.h"
#include "usb_audio.h"
#include "vfe_pipeline/vfe_pipeline.h"
#include "ww_model_runner/ww_model_runner.h"

#include "gpio_test/gpio_test.h"

#define WW_TILE_NO       0

volatile int mic_from_usb = 0;
volatile int aec_ref_source = appconfAEC_REF_DEFAULT;

void vfe_pipeline_input(void *input_app_data,
                        int32_t (*mic_audio_frame)[2],
                        int32_t (*ref_audio_frame)[2],
                        size_t frame_count)
{
    (void) input_app_data;

    static int flushed;
    while (!flushed) {
        size_t received;
        received = rtos_mic_array_rx(mic_array_ctx,
                                     mic_audio_frame,
                                     frame_count,
                                     0);
        if (received == 0) {
            rtos_mic_array_rx(mic_array_ctx,
                              mic_audio_frame,
                              frame_count,
                              portMAX_DELAY);
            flushed = 1;
        }
    }

    /*
     * NOTE: ALWAYS receive the next frame from the PDM mics,
     * even if USB is the current mic source. The controls the
     * timing since usb_audio_recv() does not block and will
     * receive all zeros if no frame is available yet.
     */
    rtos_mic_array_rx(mic_array_ctx,
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
        rtos_i2s_rx(i2s_ctx,
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
    (void) output_app_data;

#if appconfI2S_ENABLED

    int const rate_multiplier = appconfI2S_AUDIO_SAMPLE_RATE / appconfAUDIO_PIPELINE_SAMPLE_RATE;

    if (rate_multiplier == 3) {
        static int32_t src_data[2][SRC_FF3V_FIR_TAPS_PER_PHASE] __attribute__((aligned (8)));
        static int32_t i2s_audio_frames[VFE_FRAME_ADVANCE * 3][2];

        for (int i = 0; i < VFE_FRAME_ADVANCE; i++) {
            for (int j = 0; j < 2; j++) {
                i2s_audio_frames[3*i + 0][j] = src_us3_voice_input_sample(src_data[j], src_ff3v_fir_coefs[2], proc_audio_frame[i][j]);
                i2s_audio_frames[3*i + 1][j] = src_us3_voice_get_next_sample(src_data[j], src_ff3v_fir_coefs[1]);
                i2s_audio_frames[3*i + 2][j] = src_us3_voice_get_next_sample(src_data[j], src_ff3v_fir_coefs[0]);
            }
        }

        rtos_i2s_tx(i2s_ctx,
                    (int32_t *) i2s_audio_frames,
                    frame_count * rate_multiplier,
                    portMAX_DELAY);
    } else {
        rtos_i2s_tx(i2s_ctx,
                    (int32_t *) proc_audio_frame,
                    frame_count,
                    portMAX_DELAY);
    }
#endif

#if appconfUSB_ENABLED
    usb_audio_send(intertile_ctx,
                  frame_count,
                  proc_audio_frame,
                  ref_audio_frame,
                  mic_audio_frame);
#endif

#if appconfSPI_OUTPUT_ENABLED
    void spi_audio_send(rtos_intertile_t *intertile_ctx,
                        size_t frame_count,
                        int32_t (*processed_audio_frame)[2]);

    spi_audio_send(intertile_ctx,
                   frame_count,
                   proc_audio_frame);
#endif

    return VFE_PIPELINE_FREE_FRAME;
}

void vApplicationMallocFailedHook(void)
{
    rtos_printf("Malloc Failed on tile %d!\n", THIS_XCORE_TILE);
    for(;;);
}

static void mem_analysis(void)
{
	for (;;) {
		rtos_printf("Tile[%d]:\n\tMinimum heap free: %d\n\tCurrent heap free: %d\n", THIS_XCORE_TILE, xPortGetMinimumEverFreeHeapSize(), xPortGetFreeHeapSize());
		vTaskDelay(pdMS_TO_TICKS(5000));
	}
}

void startup_task(void *arg)
{
    rtos_printf("Startup task running from tile %d on core %d\n", THIS_XCORE_TILE, portGET_CORE_ID());

    platform_start();

#if ON_TILE(1)
    gpio_test(gpio_ctx_t0);
#endif

#if ON_TILE(AUDIO_HW_TILE_NO)
    vfe_pipeline_init(NULL, NULL);
#endif

#if WW && ON_TILE(WW_TILE_NO)

    StreamBufferHandle_t audio_stream = xStreamBufferCreate(
                                               1.2 * VFE_PIPELINE_AUDIO_FRAME_LENGTH,
                                               appconfWW_FRAMES_PER_INFERENCE);
    /* TODO Task that fills audio_stream with uint16_t samples */
    ww_task_create(audio_stream, appconfWW_TASK_PRIORITY);
#endif

    mem_analysis();
    /*
     * TODO: Watchdog?
     */
}

void vApplicationIdleHook(void)
{
    rtos_printf("idle hook on tile %d core %d\n", THIS_XCORE_TILE, rtos_core_id_get());
    asm volatile("waiteu");
}

static void tile_common_init(chanend_t c)
{
    control_ret_t ctrl_ret;

    platform_init(c);
    chanend_free(c);

    ctrl_ret = app_control_init();
    xassert(ctrl_ret == CONTROL_SUCCESS);

#if appconfUSB_ENABLED && ON_TILE(USB_TILE_NO)
    usb_audio_init(intertile_ctx, appconfUSB_AUDIO_TASK_PRIORITY);
#endif

    xTaskCreate((TaskFunction_t) startup_task,
                "startup_task",
                RTOS_THREAD_STACK_SIZE(startup_task),
                NULL,
                appconfSTARTUP_TASK_PRIORITY,
                NULL);

    rtos_printf("start scheduler on tile %d\n", THIS_XCORE_TILE);
    vTaskStartScheduler();
}

#if ON_TILE(0)
void main_tile0(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c0;
    (void) c2;
    (void) c3;

    tile_common_init(c1);
}
#endif

#if ON_TILE(1)
void main_tile1(chanend_t c0, chanend_t c1, chanend_t c2, chanend_t c3)
{
    (void) c1;
    (void) c2;
    (void) c3;

    tile_common_init(c0);
}
#endif
