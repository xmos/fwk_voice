// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _vad_h_
#define _vad_h_

#include <stdint.h>
#include "vad_parameters.h"
#include "vad_mel.h"

/**
 * @page page_vad_api_h vad_api.h
 * 
 * lib_vad public functions API.
 *
 * @ingroup vad_header_file
 */

/**
 * @defgroup vad_state   IC Data Structures
 */ 

/**
 * @brief IC configuration structure
 *
 * This structure contains configuration settings that can be changed to alter the
 * behaviour of the IC instance. An instance of this structure is is automatically
 * included as part of the IC state.
 * 
 * It controls the behaviour of the main filter and normalisation thereof.
 * The initial values for these configuration parameters are defined in ic_defines.h
 * and are initialised by ic_init().
 *
 * @ingroup ic_state
 */
typedef struct {
    /** History of features generated from PCM input */
    int32_t old_features[VAD_N_OLD_FEATURES];
    /** A copy of part of the old frame to build the new frame */
    int32_t prev_frame[VAD_PROC_FRAME_LENGTH - VAD_FRAME_ADVANCE];
} vad_state_t;

/**
 * @defgroup vad_func     VAD API Functions
 */ 

/** 
 * @brief Function that initialises a VAD instance. Must be called before vad_probability_voice
 *
 * @param state[inout]  Pointer to a vad_state_t state structure
 *
 * @ingroup vad_func
 */
void vad_init(vad_state_t * state);

/** 
 * @brief Function that classifies whether the recent set of samples contains voice. 
 *
 * @param input[in]            Array of samples sampled in the time domain
 *                             It should contain VAD_FRAME_ADVANCE new samples
 *                             since the last call
 *
 * @param state[inout]         Reference to the state stucture. Should be
 *                             declared as vad_state_t state.
 *                             Prior to calling the funciton, the state must
 *                             have been initialised with vad_init().
 *
 * @returns                    A number from 0 to 255, where 0 indicates
 *                             very likely not voice, and 255 indicates very
 *                             likely voice
 * 
 * @ingroup vad_func
 */
uint8_t vad_probability_voice(const int32_t input[VAD_FRAME_ADVANCE],
                            vad_state_t * state);


#endif
