// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef WW_MODEL_RUNNER_H_
#define WW_MODEL_RUNNER_H_

#include "FreeRTOS.h"
#include "stream_buffer.h"

void ww_task_create(StreamBufferHandle_t input_stream, unsigned priority);

#endif /* WW_MODEL_RUNNER_H_ */
