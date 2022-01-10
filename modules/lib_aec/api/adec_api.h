// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "adec_state.h"

/** Function that initialises the delay estimator controller (dec) state
 *
 *  \param state                    Stage A state structure
 *
 */
void adec_init(adec_state_t *state);

/** Function that takes statistics about the signals and decides if a delay
 *  estimation and/or a delay change and AEC reset is required
 *
 *  \param state                      Stage A state structure
 *
 *  \param delay_change_command_value If we need to change delay, this is set to the new (absolute) delay value
 *                                    Note: delay direction is handled separately in the state structure
 *
 *  returns                           Whether AEC and delay settings need to change
 */
void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const de_to_adec_t *de_output,
    const aec_to_adec_t *aec_to_adec,
    int32_t far_end_active_flag,
    int32_t num_frames_since_last_call
);
