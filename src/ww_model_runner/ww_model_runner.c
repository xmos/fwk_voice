// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xs1.h>
#include <xcore/hwtimer.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "app_conf.h"
#include "platform/driver_instances.h"
#include "ww_model_runner/ww_model_runner.h"

#if appconfWW_ENABLED

#include "pryon_lite.h"

// sizes below are based on verion v2.10.6
// #define MODEL_RUNNER_STACK_SIZE (3 * 1024)
// #define MODEL_RUNNER_CODE_SIZE (43 * 1024)
// #define MODEL_RUNNER_DECODE_BUFFER_SIZE (30 * 1024)
// #define MODEL_RUNNER_MODEL_DATA_SIZE (234 * 1024)
// #define WW_MODEL_RUNTICKS 1250000

#define MODEL_RUNNER_STACK_SIZE (650)  // in WORDS
#define DECODER_BUF_SIZE (30000)  // (29936)  for v2.10.6

extern char prlBinaryModelData[];
extern size_t prlBinaryModelLen;

static PryonLiteDecoderHandle sDecoder = NULL;

__attribute__((aligned(8))) static uint8_t decoder_buf[DECODER_BUF_SIZE];
static uint8_t *decoder_buf_ptr = (uint8_t *)&decoder_buf;

static void detectionCallback(PryonLiteDecoderHandle handle,
                              const PryonLiteResult *result)
{
    rtos_printf(
        "Detected: keyword='%s'  confidence=%d  beginSampleIndex=%lld  "
        "endSampleIndex=%lld\n",
        result->keyword, result->confidence, result->beginSampleIndex,
        result->endSampleIndex);
}

static void vadCallback(PryonLiteDecoderHandle handle,
                        const PryonLiteVadEvent *vadEvent)
{
    rtos_printf("VAD state: %s\n", vadEvent->vadState ? "active" : "inactive");
}


static void model_runner_manager(void *args)
{
    StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;

    rtos_printf("model size is %d bytes\n", prlBinaryModelLen);
    /* load model */
    PryonLiteDecoderConfig config = PryonLiteDecoderConfig_Default;
    config.sizeofModel = prlBinaryModelLen;
    config.model = prlBinaryModelData;

    // Query for the size of instance memory required by the decoder
    PryonLiteModelAttributes modelAttributes;
    PryonLiteError status = PryonLite_GetModelAttributes(
        config.model, config.sizeofModel, &modelAttributes);

    rtos_printf("required decoder buf size %d bytes\n",
                modelAttributes.requiredDecoderMem);
    config.decoderMem = (char *)decoder_buf_ptr;
    config.sizeofDecoderMem = modelAttributes.requiredDecoderMem;

    // initialize decoder
    PryonLiteSessionInfo sessionInfo;

    config.detectThreshold = 500;               // default threshold
    config.resultCallback = detectionCallback;  // register detection handler
    config.vadCallback = vadCallback;           // register VAD handler
    config.useVad = 1;                          // enable voice activity detector

    status = PryonLiteDecoder_Initialize(&config, &sessionInfo, &sDecoder);

    if (status != PRYON_LITE_ERROR_OK)
    {
        rtos_printf("Failed to initialize pryon lite decoder!\n");
        while (1);
    }

    // Set detection threshold for all keywords (this function can be called any
    // time after decoder initialization)
    int detectionThreshold = 500;
    status = PryonLiteDecoder_SetDetectionThreshold(sDecoder, NULL,
                                                    detectionThreshold);

    if (status != PRYON_LITE_ERROR_OK)
    {
        rtos_printf("Failed to set pryon lite threshold!\n");
        while (1);
    }

    rtos_printf("samples per frame should be %d\n", sessionInfo.samplesPerFrame);

    rtos_printf("\tMinimum heap free: %d\n", xPortGetMinimumEverFreeHeapSize());
    rtos_printf("\tCurrent heap free: %d\n", xPortGetFreeHeapSize());
    rtos_printf("\tfree stack words: %d\n", uxTaskGetStackHighWaterMark(NULL));

    configASSERT(sessionInfo.samplesPerFrame == appconfWW_FRAMES_PER_INFERENCE);

    int16_t buf[appconfWW_FRAMES_PER_INFERENCE];

    while (1)
    {
        uint8_t *buf_ptr = (uint8_t*)buf;
        size_t buf_len = appconfWW_FRAMES_PER_INFERENCE * sizeof(int16_t);
        do {
            size_t bytes_rxed = xStreamBufferReceive(input_queue,
                                                     buf_ptr,
                                                     buf_len,
                                                     portMAX_DELAY);
            buf_len -= bytes_rxed;
            buf_ptr += bytes_rxed;
        } while(buf_len > 0);

        if (sDecoder != NULL)
        {
            status = PryonLiteDecoder_PushAudioSamples(sDecoder, buf,
            sessionInfo.samplesPerFrame);

            if (status != PRYON_LITE_ERROR_OK)
            {
                rtos_printf("Error while pushing new samples!\n");
                while (1);
            }
        }
    }
}

void ww_audio_send(rtos_intertile_t *intertile_ctx,
                    size_t frame_count,
                    int32_t (*processed_audio_frame)[2])
{
    configASSERT(frame_count == appconfAUDIO_PIPELINE_FRAME_ADVANCE);

    uint16_t ww_samples[appconfAUDIO_PIPELINE_FRAME_ADVANCE];

    for (int i = 0; i < frame_count; i++) {
        ww_samples[i] = (uint16_t)(processed_audio_frame[i][0] >> 16);
    }

    rtos_intertile_tx(intertile_ctx,
                      appconfWW_SAMPLES_PORT,
                      ww_samples,
                      sizeof(ww_samples));
}

void ww_samples_in_task(void *arg)
{
    StreamBufferHandle_t samples_to_ww_stream_buf = (StreamBufferHandle_t) arg;

    for (;;) {
        uint16_t ww_samples[appconfAUDIO_PIPELINE_FRAME_ADVANCE];
        size_t bytes_received;

        bytes_received = rtos_intertile_rx_len(
                intertile_ctx,
                appconfWW_SAMPLES_PORT,
                portMAX_DELAY);

        xassert(bytes_received == sizeof(ww_samples));

        rtos_intertile_rx_data(
                intertile_ctx,
                ww_samples,
                bytes_received);

        if (xStreamBufferSend(samples_to_ww_stream_buf, ww_samples, sizeof(ww_samples), 0) != sizeof(ww_samples)) {
            rtos_printf("lost output samples for ww\n");
        }
    }
}

void ww_task_create(unsigned priority)
{
    StreamBufferHandle_t audio_stream = xStreamBufferCreate(
                                               2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE,
                                               appconfWW_FRAMES_PER_INFERENCE);

    xTaskCreate((TaskFunction_t)ww_samples_in_task,
                "ww_audio_rx",
                RTOS_THREAD_STACK_SIZE(ww_samples_in_task),
                audio_stream,
                priority-1,
                NULL);

    xTaskCreate((TaskFunction_t)model_runner_manager,
                "model_manager",
                MODEL_RUNNER_STACK_SIZE,
                audio_stream,
                priority,
                NULL);
}

#endif
