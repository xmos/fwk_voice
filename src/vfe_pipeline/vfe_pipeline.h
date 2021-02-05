// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef VFE_PIPELINE_H_
#define VFE_PIPELINE_H_

#include "rtos/drivers/mic_array/api/rtos_mic_array.h"
#include "rtos/drivers/i2s/api/rtos_i2s_master.h"

#include "voice_front_end_settings.h"

#define VFE_PIPELINE_AUDIO_SAMPLE_RATE 16000
#define VFE_PIPELINE_AUDIO_FRAME_LENGTH VFE_FRAME_ADVANCE

#define VFE_PIPELINE_DONT_FREE_FRAME 0
#define VFE_PIPELINE_FREE_FRAME      1

void vfe_pipeline_init(
        void *input_app_data,
        void *output_app_data);

void vfe_pipeline_input(void *input_app_data,
                        int32_t (*audio_frame)[2],
                        size_t frame_count);

int vfe_pipeline_output(void *output_app_data,
                        int32_t (*audio_frame)[2],
                        size_t frame_count);

#endif /* VFE_PIPELINE_H_ */
