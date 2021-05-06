// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <xcore/hwtimer.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_conf.h"
#include "ww_model_runner/ww_model_runner.h"

#define DUMMY_TESTING 1

#if DUMMY_TESTING
#define MODEL_RUNNER_STACK_SIZE (300000>>2)
#else
#define MODEL_RUNNER_STACK_SIZE 650
#endif

#define WW_MODEL_RUNTICKS    1250000

static void model_runner_manager(void *args)
{
    StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;

    int16_t buf[appconfWW_SAMPLES_PER_FRAME];
    while(1) {
        xStreamBufferReceive(input_queue, buf, appconfWW_SAMPLES_PER_FRAME * sizeof(int16_t), portMAX_DELAY);
    	uint32_t init_time = get_reference_time();
    	while (get_reference_time() - init_time < WW_MODEL_RUNTICKS);
    }
}

void ww_task_create(StreamBufferHandle_t input_stream, unsigned priority)
{
    xTaskCreate((TaskFunction_t) model_runner_manager,
                "model_manager",
                MODEL_RUNNER_STACK_SIZE,
                input_stream,
                priority,
                NULL);
}
