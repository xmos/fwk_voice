// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef _voice_front_end_stages_
#define _voice_front_end_stages_

#include "voice_front_end_settings.h"
#include "vfe_dsp_state.h"
#include "voice_toolbox.h"


/** Initialise DSP state for stage 1
 *
 * \param[out] dsp_state
 */
void init_dsp_stage_1(vfe_dsp_stage_1_state_t *dsp_state);

/** Initialise DSP state for stage 2
 *
 * \param[out] dsp_state
 */
void init_dsp_stage_2(vfe_dsp_stage_2_state_t *dsp_state);

/** Voice Front End: Process a single frame with stage 1
 *
 * NOTE: The post_proc_frame argument is a double pointer. When dsp_stage_1_process
 * returns, this points to an array stored in the vfe_dsp_stage_1_state_t structure.
 * See example code for clarification.
 *
 * \param dsp_state             Must be initialised using init_dsp_stage_1()
 * \param input_frame           Input audio frame: vtb_ch_pair_t[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE]
 * \param[out] meta_data        Meta-data to pass to stage 2
 * \param[out] post_proc_frame  Pointer to an array of type:
 *                              vtb_ch_pair_t[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE]
 */
void dsp_stage_1_process(
    vfe_dsp_stage_1_state_t *dsp_state,
    vtb_ch_pair_t *input_frame,
    vtb_md_t *meta_data,
    vtb_ch_pair_t **post_proc_frame
);

/** Voice Front End: Process a single frame with stage 2
 *
 * NOTE: The post_proc_frame argument is a 2D array. In contrast to post_proc_frame in
 * dsp_stage_1_process, this function requires that memory is allocated specifically for
 * post_proc_frame before calling this function. See example code for clarification.
 *
 * \param dsp_state             Must be initialised using init_dsp_stage_2()
 * \param input_frame           Input audio frame: vtb_ch_pair_t[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE]
 * \param stage_1_md            Meta-data from stage_1
 * \param[out] post_proc_frame  Output audio frame
 */
void dsp_stage_2_process(
    vfe_dsp_stage_2_state_t *dsp_state,
    vtb_ch_pair_t *input_frame,
    vtb_md_t stage_1_md,
    vtb_ch_pair_t post_proc_frame[VFE_CHANNEL_PAIRS][VFE_FRAME_ADVANCE]
);

#endif // _voice_front_end_stages_
