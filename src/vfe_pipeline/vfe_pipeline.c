// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <string.h>
#include <xcore/hwtimer.h>

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "voice_front_end.h"

#include "app_conf.h"
#include "app_control/app_control.h"
#include "audio_pipeline/audio_pipeline.h"
#include "vfe_pipeline.h"

static vfe_dsp_stage_1_state_t dsp_stage_1_state;
static vfe_dsp_stage_2_state_t dsp_stage_2_state;

typedef struct
{
    vtb_ch_pair_t samples[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE];

    vtb_ch_pair_t mic_samples_passthrough[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE];
    vtb_ch_pair_t aec_reference_audio_samples[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE];

    vtb_md_t stage_1_md;
} frame_data_t;

static void *vfe_pipeline_input_i(void *input_app_data)
{
    frame_data_t *frame_data;

    frame_data = pvPortMalloc(sizeof(frame_data_t));

    vfe_pipeline_input(input_app_data,
                       (int32_t(*)[2])frame_data->samples,
                       (int32_t(*)[2])frame_data->aec_reference_audio_samples,
                       VFE_FRAME_ADVANCE);

    memcpy(frame_data->mic_samples_passthrough, frame_data->samples, sizeof(frame_data->mic_samples_passthrough));

    return frame_data;
}

static int vfe_pipeline_output_i(frame_data_t *frame_data,
                                 void *output_app_data)
{
    return vfe_pipeline_output(output_app_data,
                               (int32_t(*)[2])frame_data->samples,
                               (int32_t(*)[2])frame_data->mic_samples_passthrough,
                               (int32_t(*)[2])frame_data->aec_reference_audio_samples,
                               VFE_FRAME_ADVANCE);
}

/* Implement an dummy AEC stage */

/* Memory: 186kB per AEC-less-Stage A */
#define DUMMY_AEC_DATA (186000) // should be 290000
volatile static uint8_t waste[DUMMY_AEC_DATA];

/* Threading:
 * AEC currently uses 5 threads for parallel steps
 * and requires 300 MIPS total
 * Audio pipeline runs at 16kHz with a 240 frame advance
 * split evenly among cores for now, but main core has additional work
 */
#define AEC_MIPS_REQUIRED 300
#define AEC_THREADS 3
#define AEC_THREAD_PRIO (configMAX_PRIORITIES - 1)

#define AEC_TOTAL_CYCLE_COUNT ((AEC_MIPS_REQUIRED * 1000000 / VFE_PIPELINE_AUDIO_SAMPLE_RATE) * VFE_PIPELINE_AUDIO_FRAME_LENGTH)
#define AEC_DUMMY_BURN_TICKS (AEC_TOTAL_CYCLE_COUNT / AEC_THREADS)

#include "event_groups.h"
#include "burn_cycles.h"

typedef struct
{
    EventGroupHandle_t sync_group_start;
    EventGroupHandle_t sync_group_end;
} vfe_dsp_stage_0_state_t;

typedef struct
{
    vfe_dsp_stage_0_state_t *stage_state;
    int id;
} aec_dummy_args_t;

static vfe_dsp_stage_0_state_t dsp_stage_0_state;

static void aec_burn_dummy(aec_dummy_args_t *args)
{
    EventGroupHandle_t sync_group_start = args->stage_state->sync_group_start;
    EventGroupHandle_t sync_group_end = args->stage_state->sync_group_end;
    int id = args->id;
    uint32_t all_sync_bits = 0;

    for (int i = 0; i <= AEC_THREADS; i++)
    {
        all_sync_bits |= 1 << i;
    }

    while (1)
    {
        xEventGroupSync(sync_group_start, 1 << id, all_sync_bits, portMAX_DELAY);
        // uint32_t init_time = get_reference_time();
        burn_cycles(AEC_DUMMY_BURN_TICKS);
        // rtos_printf("duration of thread %d %u @ %u\n", id, get_reference_time()-init_time, get_reference_time());
        xEventGroupSync(sync_group_end, 1 << id, all_sync_bits, portMAX_DELAY);
    }
}

static void init_dsp_stage_0(vfe_dsp_stage_0_state_t *state)
{
    dsp_stage_0_state.sync_group_start = xEventGroupCreate();
    dsp_stage_0_state.sync_group_end = xEventGroupCreate();
    uint32_t all_sync_bits = 0;
    waste[0] = 0;

    for (int i = 0; i <= AEC_THREADS; i++)
    {
        all_sync_bits |= 1 << i;
    }

    xEventGroupClearBits(dsp_stage_0_state.sync_group_start, all_sync_bits);
    xEventGroupClearBits(dsp_stage_0_state.sync_group_end, all_sync_bits);

    for (int i = 0; i < AEC_THREADS; i++)
    {
        aec_dummy_args_t *args = pvPortMalloc(sizeof(aec_dummy_args_t));
        args->stage_state = &dsp_stage_0_state;
        args->id = i + 1;

        xTaskCreate((TaskFunction_t)aec_burn_dummy,
                    "aec_dummy",
                    RTOS_THREAD_STACK_SIZE(aec_burn_dummy),
                    args,
                    AEC_THREAD_PRIO,
                    NULL);
    }
}

DEVICE_CONTROL_CALLBACK_ATTR
control_ret_t read_cmd(control_resid_t resid, control_cmd_t cmd, uint8_t *payload, size_t payload_len, void *app_data)
{
    rtos_printf("Device control READ\n\t");

    rtos_printf("Servicer on tile %d received command %02x for resid %02x\n\t", THIS_XCORE_TILE, cmd, resid);
    rtos_printf("The command is requesting %d bytes\n\t", payload_len);
    for (int i = 0; i < payload_len; i++)
    {
        payload[i] = (cmd & 0x7F) + i;
    }
    rtos_printf("Bytes to be sent are:\n\t", payload_len);
    for (int i = 0; i < payload_len; i++)
    {
        rtos_printf("%02x ", payload[i]);
    }
    rtos_printf("\n\n");

    return CONTROL_SUCCESS;
}

DEVICE_CONTROL_CALLBACK_ATTR
control_ret_t write_cmd(control_resid_t resid, control_cmd_t cmd, const uint8_t *payload, size_t payload_len, void *app_data)
{
    rtos_printf("Device control WRITE\n\t");

    rtos_printf("Servicer on tile %d received command %02x for resid %02x\n\t", THIS_XCORE_TILE, cmd, resid);
    rtos_printf("The command has %d bytes\n\t", payload_len);
    rtos_printf("Bytes received are:\n\t", payload_len);
    for (int i = 0; i < payload_len; i++)
    {
        rtos_printf("%02x ", payload[i]);
    }
    rtos_printf("\n\n");

    return CONTROL_SUCCESS;
}

static void stage0(frame_data_t *frame_data)
{
    static int init;
    static device_control_servicer_t servicer_ctx;

    if (!init)
    {
        const control_resid_t resources[] = {'A', 'E', 'C'};
        control_ret_t dc_ret;

        rtos_printf("Will register the AEC servicer now\n");

        dc_ret = app_control_servicer_register(&servicer_ctx,
                                               resources, sizeof(resources));
        xassert(dc_ret == CONTROL_SUCCESS);
        rtos_printf("AEC servicer registered\n");

        init = 1;
    }

    EventBits_t all_sync_bits = 0;

    device_control_servicer_cmd_recv(&servicer_ctx, read_cmd, write_cmd, NULL, 0);

    for (int i = 0; i <= AEC_THREADS; i++)
    {
        all_sync_bits |= 1 << i;
    }

    xEventGroupSync(dsp_stage_0_state.sync_group_start, 1, all_sync_bits, portMAX_DELAY);
    xEventGroupClearBits(dsp_stage_0_state.sync_group_start, all_sync_bits);

    // Wait for workers to finish
    xEventGroupSync(dsp_stage_0_state.sync_group_end, 1, all_sync_bits, portMAX_DELAY);
    xEventGroupClearBits(dsp_stage_0_state.sync_group_end, all_sync_bits);

    memcpy(frame_data->samples,
           frame_data->samples,
           sizeof(frame_data->samples));
}

static void stage1(frame_data_t *frame_data)
{
    vtb_ch_pair_t *post_proc_frame = NULL;

    dsp_stage_1_process(&dsp_stage_1_state,
                        (vtb_ch_pair_t *)frame_data->samples,
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
                        (vtb_ch_pair_t *)frame_data->samples,
                        frame_data->stage_1_md,
                        frame_data->samples);
}

void vfe_pipeline_init(
    void *input_app_data,
    void *output_app_data)
{
    const int stage_count = 3;

    const audio_pipeline_stage_t stages[] = {
        (audio_pipeline_stage_t)stage0,
        (audio_pipeline_stage_t)stage1,
        (audio_pipeline_stage_t)stage2,
    };

    const configSTACK_DEPTH_TYPE stage_stack_sizes[] = {
        configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage0) + RTOS_THREAD_STACK_SIZE(vfe_pipeline_input_i),
        configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage1),
        configMINIMAL_STACK_SIZE + RTOS_THREAD_STACK_SIZE(stage2) + RTOS_THREAD_STACK_SIZE(vfe_pipeline_output_i),
    };

    init_dsp_stage_0(&dsp_stage_0_state);
    init_dsp_stage_1(&dsp_stage_1_state);
    init_dsp_stage_2(&dsp_stage_2_state);

    audio_pipeline_init((audio_pipeline_input_t)vfe_pipeline_input_i,
                        (audio_pipeline_output_t)vfe_pipeline_output_i,
                        input_app_data,
                        output_app_data,
                        stages,
                        stage_stack_sizes,
                        configMAX_PRIORITIES / 2,
                        stage_count);
}
