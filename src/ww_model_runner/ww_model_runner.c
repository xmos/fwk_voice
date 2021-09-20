// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>
#include <xcore/hwtimer.h>
#include <xs1.h>

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

#include "app_conf.h"
#include "platform/driver_instances.h"
#include "ww_model_runner/ww_model_runner.h"

#if appconfWW_ENABLED

#include "fs_support.h"
#include "pryon_lite.h"

#define ASR_CHANNEL (0)
#define COMMS_CHANNEL (1)

#if (WW_250K == 1)
#define WW_MODEL_FILEPATH "/flash/ww/250kenUS.bin"
#define MODEL_RUNNER_STACK_SIZE (670) // in WORDS
#define DECODER_BUF_SIZE (30000)      // (29936)  for v2.10.6
#define WW_MAX_SIZE_BYTES (233528)    // 234k model size + 0% headroom
#else
#define WW_MODEL_FILEPATH "/flash/ww/50kenUS.bin"
#define MODEL_RUNNER_STACK_SIZE (670) // in WORDS
#define DECODER_BUF_SIZE (30000)      // (27960)  for v2.5.0
#define WW_MAX_SIZE_BYTES (60000)     // 50k model size + 20% headroom
#endif                                /* (WW_250K == 1) */

static uint8_t *prlBinaryModelPtr = NULL;

#if (WW_MODEL_IN_SRAM == 1)
static size_t prlBinaryModelLen = 0;
char prlBinaryModelData[WW_MAX_SIZE_BYTES] = {0};
#else
extern size_t prlBinaryModelLen;
extern char prlBinaryModelData[];
#endif /* (WW_MODEL_IN_SRAM == 1) */

static PryonLiteDecoderHandle sDecoder = NULL;

__attribute__((aligned(8))) static uint8_t decoder_buf[DECODER_BUF_SIZE];
static uint8_t *decoder_buf_ptr = (uint8_t *)&decoder_buf;

#define WW_ERROR_LED 0
#define WW_VAD_LED 1
#define WW_ALEXA_LED 2
#define set_led(led, val)                                                      \
  {                                                                            \
    const rtos_gpio_port_id_t led_port = rtos_gpio_port(PORT_LEDS);            \
    uint32_t cur_val;                                                          \
    cur_val = rtos_gpio_port_in(gpio_ctx_t0, led_port);                        \
    rtos_gpio_port_out(gpio_ctx_t0, led_port,                                  \
                       (val == 0) ? cur_val & ~(1 << led)                      \
                                  : (cur_val | (1 << led)));                   \
  }

static void detectionCallback(PryonLiteDecoderHandle handle,
                              const PryonLiteResult *result) {
  set_led(WW_ALEXA_LED, 1);
  rtos_printf(
      "Wakeword Detected: keyword='%s'  confidence=%d  beginSampleIndex=%lld  "
      "endSampleIndex=%lld\n",
      result->keyword, result->confidence, result->beginSampleIndex,
      result->endSampleIndex);
  rtos_printf("\tww free stack words: %d\n", uxTaskGetStackHighWaterMark(NULL));
}

static void vadCallback(PryonLiteDecoderHandle handle,
                        const PryonLiteVadEvent *vadEvent) {
  rtos_printf("Wakeword VAD: state=%s\n",
              vadEvent->vadState ? "active" : "inactive");

  if (vadEvent->vadState == 0) {
    set_led(WW_ALEXA_LED, 0);
    set_led(WW_VAD_LED, 0);
  } else {
    set_led(WW_VAD_LED, 1);
  }
  rtos_printf("\tvad free stack words: %d\n",
              uxTaskGetStackHighWaterMark(NULL));
}

static size_t ww_load_model_from_fs(void) {
  size_t retval = 0;
  FIL ww_model_file;
  uint32_t ww_model_file_size = -1;
  uint32_t bytes_read = 0;
  FRESULT result;

  result = f_open(&ww_model_file, WW_MODEL_FILEPATH, FA_READ);
  if (result == FR_OK) {
    rtos_printf("Found model %s\n", WW_MODEL_FILEPATH);
    ww_model_file_size = f_size(&ww_model_file);

    result = f_read(&ww_model_file, (uint8_t *)prlBinaryModelData,
                    ww_model_file_size, (unsigned int *)&bytes_read);
    configASSERT(bytes_read == ww_model_file_size);
    prlBinaryModelPtr = (uint8_t *)prlBinaryModelData;

  } else {
    rtos_printf("Failed to open model %s\n", WW_MODEL_FILEPATH);
  }
  if (ww_model_file_size != -1) {
    rtos_printf("Loaded model %s\n", WW_MODEL_FILEPATH);
    f_close(&ww_model_file);
    retval = ww_model_file_size;
  }

  return retval;
}

static void model_runner_manager(void *args) {
  StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;

  rtos_printf("model size is %d bytes\n", prlBinaryModelLen);
  /* load model */
  PryonLiteDecoderConfig config = PryonLiteDecoderConfig_Default;
  config.sizeofModel = prlBinaryModelLen;
  config.model = prlBinaryModelPtr;

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

  config.detectThreshold = 500;              // default threshold
  config.resultCallback = detectionCallback; // register detection handler
  config.vadCallback = vadCallback;          // register VAD handler
  config.useVad = 1;                         // enable voice activity detector

  status = PryonLiteDecoder_Initialize(&config, &sessionInfo, &sDecoder);

  if (status != PRYON_LITE_ERROR_OK) {
    rtos_printf("Failed to initialize pryon lite decoder!\n");
    set_led(WW_ERROR_LED, 1);
    while (1)
      ;
  }

  // Set detection threshold for all keywords (this function can be called any
  // time after decoder initialization)
  int detectionThreshold = 500;
  status = PryonLiteDecoder_SetDetectionThreshold(sDecoder, NULL,
                                                  detectionThreshold);

  if (status != PRYON_LITE_ERROR_OK) {
    rtos_printf("Failed to set pryon lite threshold!\n");
    set_led(WW_ERROR_LED, 1);
    while (1)
      ;
  }

  rtos_printf("samples per frame should be %d\n", sessionInfo.samplesPerFrame);

  rtos_printf("\tMinimum heap free: %d\n", xPortGetMinimumEverFreeHeapSize());
  rtos_printf("\tCurrent heap free: %d\n", xPortGetFreeHeapSize());
  rtos_printf("\tfree stack words: %d\n", uxTaskGetStackHighWaterMark(NULL));

  configASSERT(sessionInfo.samplesPerFrame == appconfWW_FRAMES_PER_INFERENCE);

  int16_t buf[appconfWW_FRAMES_PER_INFERENCE];

  while (1) {
    uint8_t *buf_ptr = (uint8_t *)buf;
    size_t buf_len = appconfWW_FRAMES_PER_INFERENCE * sizeof(int16_t);
    do {
      size_t bytes_rxed =
          xStreamBufferReceive(input_queue, buf_ptr, buf_len, portMAX_DELAY);
      buf_len -= bytes_rxed;
      buf_ptr += bytes_rxed;
    } while (buf_len > 0);

    if (sDecoder != NULL) {

      // uint32_t tic = get_reference_time(); // TODO: Delete me!

      status = PryonLiteDecoder_PushAudioSamples(sDecoder, buf,
                                                 sessionInfo.samplesPerFrame);

      // rtos_printf("\tPryonLiteDecoder_PushAudioSamples: duration=%lu us\n",
      //             (get_reference_time() - tic) /
      //                 PLATFORM_REFERENCE_MHZ); // TODO: Delete me!

      if (status != PRYON_LITE_ERROR_OK) {
        set_led(WW_ERROR_LED, 1);
        rtos_printf("Error while pushing new samples!\n");
        while (1)
          ;
      }
    }
  }
}

void ww_audio_send(rtos_intertile_t *intertile_ctx, size_t frame_count,
                   int32_t (*processed_audio_frame)[2]) {
  configASSERT(frame_count == appconfAUDIO_PIPELINE_FRAME_ADVANCE);

  uint16_t ww_samples[appconfAUDIO_PIPELINE_FRAME_ADVANCE];

  for (int i = 0; i < frame_count; i++) {
    ww_samples[i] = (uint16_t)(processed_audio_frame[i][ASR_CHANNEL] >> 16);
  }

  rtos_intertile_tx(intertile_ctx, appconfWW_SAMPLES_PORT, ww_samples,
                    sizeof(ww_samples));
}

void ww_samples_in_task(void *arg) {
  StreamBufferHandle_t samples_to_ww_stream_buf = (StreamBufferHandle_t)arg;

  for (;;) {
    uint16_t ww_samples[appconfAUDIO_PIPELINE_FRAME_ADVANCE];
    size_t bytes_received;

    bytes_received = rtos_intertile_rx_len(
        intertile_ctx, appconfWW_SAMPLES_PORT, portMAX_DELAY);

    xassert(bytes_received == sizeof(ww_samples));

    rtos_intertile_rx_data(intertile_ctx, ww_samples, bytes_received);

    if (xStreamBufferSend(samples_to_ww_stream_buf, ww_samples,
                          sizeof(ww_samples), 0) != sizeof(ww_samples)) {
      rtos_printf("lost output samples for ww\n");
    }
  }
}

static void ww_load_from_fs(void *args) {
  StreamBufferHandle_t input_queue = (StreamBufferHandle_t)args;

#if (WW_MODEL_IN_SRAM == 1)
  prlBinaryModelLen = ww_load_model_from_fs();
#else
  prlBinaryModelPtr = (uint8_t *)(prlBinaryModelData);
#endif
  xTaskCreate((TaskFunction_t)model_runner_manager, "model_manager",
              MODEL_RUNNER_STACK_SIZE, input_queue, uxTaskPriorityGet(NULL),
              NULL);

  vTaskDelete(NULL);
  while (1)
    ;
}

void ww_task_create(unsigned priority) {
  StreamBufferHandle_t audio_stream = xStreamBufferCreate(
      2 * appconfAUDIO_PIPELINE_FRAME_ADVANCE, appconfWW_FRAMES_PER_INFERENCE);
  const rtos_gpio_port_id_t led_port = rtos_gpio_port(PORT_LEDS);
  rtos_gpio_port_enable(gpio_ctx_t0, led_port);
  rtos_gpio_port_out(gpio_ctx_t0, led_port, 0);

  xTaskCreate((TaskFunction_t)ww_samples_in_task, "ww_audio_rx",
              RTOS_THREAD_STACK_SIZE(ww_samples_in_task), audio_stream,
              priority - 1, NULL);

  xTaskCreate((TaskFunction_t)ww_load_from_fs, "ww_load_to_extmem",
              RTOS_THREAD_STACK_SIZE(ww_load_from_fs), audio_stream, priority,
              NULL);
}

#endif
