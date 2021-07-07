// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <xcore/hwtimer.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_conf.h"
#include "ww_model_runner/ww_model_runner.h"

#if WW
// sizes below are based on verion v2.10.6
#define MODEL_RUNNER_STACK_SIZE (3 * 1024)
#define MODEL_RUNNER_CODE_SIZE (43 * 1024)
#define MODEL_RUNNER_DECODE_BUFFER_SIZE (30 * 1024)
#define MODEL_RUNNER_MODEL_DATA_SIZE (234 * 1024)
#if ON_TILE(0)
volatile static uint8_t waste[MODEL_RUNNER_DECODE_BUFFER_SIZE + MODEL_RUNNER_MODEL_DATA_SIZE + MODEL_RUNNER_CODE_SIZE];
#endif

#define WW_MODEL_RUNTICKS 1250000

static void model_runner_manager(void *args)
{
#if ON_TILE(0)
    waste[0] = 0;
#endif
    StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;

    int16_t buf[appconfWW_FRAMES_PER_INFERENCE];
    while (1)
    {
        xStreamBufferReceive(input_queue, buf, appconfWW_FRAMES_PER_INFERENCE * sizeof(int16_t), portMAX_DELAY);
        uint32_t init_time = get_reference_time();
        while (get_reference_time() - init_time < WW_MODEL_RUNTICKS)
            ;
    }
}

void ww_task_create(StreamBufferHandle_t input_stream, unsigned priority)
{
    xTaskCreate((TaskFunction_t)model_runner_manager,
                "model_manager",
                MODEL_RUNNER_STACK_SIZE,
                input_stream,
                priority,
                NULL);
}

#endif
