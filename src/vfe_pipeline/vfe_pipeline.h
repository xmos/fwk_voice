// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef VFE_PIPELINE_H_
#define VFE_PIPELINE_H_

#include "rtos/drivers/mic_array/api/rtos_mic_array.h"
#include "rtos/drivers/i2s/api/rtos_i2s_master.h"

#include "voice_front_end_settings.h"

#define VFE_PIPELINE_AUDIO_SAMPLE_RATE 16000
#define VFE_PIPELINE_AUDIO_FRAME_LENGTH VFE_FRAME_ADVANCE

void vfe_pipeline_init(
        rtos_mic_array_t *mic_array_ctx,
        rtos_i2s_master_t *i2s_master_ctx);

void vfe_pipeline_input(rtos_mic_array_t *mic_array_ctx,
                        int32_t (*audio_frame)[2],
                        size_t frame_count);

void vfe_pipeline_output(rtos_i2s_master_t *i2s_master_ctx,
                         int32_t (*audio_frame)[2],
                         size_t frame_count);

#endif /* VFE_PIPELINE_H_ */
