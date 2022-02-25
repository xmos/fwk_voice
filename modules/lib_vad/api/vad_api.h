// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_h_
#define _vad_h_

#include <stdint.h>
#include "vad_parameters.h"
#include "vad_mel.h"

typedef struct {
    int32_t old_features[VAD_N_OLD_FEATURES];
    int32_t prev_frame[VAD_PROC_FRAME_LENGTH - VAD_FRAME_ADVANCE];
} vad_state_t;

/** Function that initialises a VAD. Must be called before vad_probability_voice
 *
 * \param state                Reference to a VAD state structure].
 *
 */
void vad_init(vad_state_t * state);

/** Function that classifies whether a current set of samples is voice or not. 
 *
 * \param input                array of samples sampled in the time domain
 *                             It should contain VAD_FRAME_ADVANCE new samples
 *                             since the last call
 *
 * \param state                Reference to the state stucture. Should be
 *                             declared as vad_state_t state.
 *                             Prior to calling the funciton, the state must
 *                             have been initialised with vad_init().
 *
 * \returns                    A number from 0 to 255, where 0 indicates
 *                             very likely not voice, and 255 indicates very
 *                             likely voice
 */
int32_t vad_probability_voice(const int32_t input[VAD_FRAME_ADVANCE],
                            vad_state_t * state);


#endif