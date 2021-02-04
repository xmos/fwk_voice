// Copyright (c) 2020, XMOS Ltd, All rights reserved

#include <string.h>
#include <xcore/hwtimer.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "audio_pipeline/audio_pipeline.h"
#include "vfe_pipeline.h"

#include "voice_front_end.h"

static vfe_dsp_stage_1_state_t dsp_stage_1_state;
static vfe_dsp_stage_2_state_t dsp_stage_2_state;

typedef struct {
    vtb_ch_pair_t samples[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE];
    vtb_md_t stage_1_md;
} frame_data_t;

static void* vfe_pipeline_input_i(rtos_mic_array_t *mic_array_ctx)
{
    frame_data_t *frame_data;

    frame_data = pvPortMalloc(sizeof(frame_data_t));

    vfe_pipeline_input(mic_array_ctx,
                       (int32_t (*)[2]) frame_data->samples,
                       VFE_FRAME_ADVANCE);

    return frame_data;
}

static int vfe_pipeline_output_i(frame_data_t *frame_data,
                                 rtos_i2s_master_t *i2s_master_ctx)
{
    vfe_pipeline_output(i2s_master_ctx,
                        (int32_t (*)[2]) frame_data->samples,
                        VFE_FRAME_ADVANCE);

    vPortFree(frame_data);

    return 0;
}

static void stage1(frame_data_t *frame_data)
{
    vtb_ch_pair_t *post_proc_frame = NULL;

    dsp_stage_1_process(&dsp_stage_1_state,
                        (vtb_ch_pair_t*) frame_data->samples,
                        &frame_data->stage_1_md,
                        &post_proc_frame);

    memcpy(frame_data->samples,
           post_proc_frame,
           sizeof(frame_data->samples));
}

static void stage2(frame_data_t *frame_data)
{
    /*
     * The output frame can apparently be the input frame,
     * provided that VFE_CHANNEL_PAIRS == 1
     */
    dsp_stage_2_process(&dsp_stage_2_state,
                        (vtb_ch_pair_t*) frame_data->samples,
                        frame_data->stage_1_md,
                        frame_data->samples);
}

void vfe_pipeline_init(rtos_mic_array_t *mic_array_ctx,
                       rtos_i2s_master_t *i2s_master_ctx)
{
    const int stage_count = 2;

    const audio_pipeline_stage_t stages[stage_count] = {(audio_pipeline_stage_t) stage1, (audio_pipeline_stage_t) stage2, };

    const configSTACK_DEPTH_TYPE stage_stack_sizes[stage_count] = {
    configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage1),
    configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage2), };

    init_dsp_stage_1(&dsp_stage_1_state);
    init_dsp_stage_2(&dsp_stage_2_state);

    audio_pipeline_init((audio_pipeline_input_t) vfe_pipeline_input_i,
                        (audio_pipeline_output_t) vfe_pipeline_output_i,
                        mic_array_ctx,
                        i2s_master_ctx,
                        stages,
                        stage_stack_sizes,
                        configMAX_PRIORITIES / 2,
                        stage_count);
}
