// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef SUP_API_H
#define SUP_API_H

#include "suppression_state.h"

/** Function that resets the noise suppressor. Does not affect
 * any parameters that have been set, such as enable, alphas etc.
 *
 * \param[in,out] state      Suppressor state, initialised
 */
void sup_reset_noise_suppression(sup_state_t * state);



/** Function that initialises the suppression state.
 *
 * It initialises the noise suppressor with the following settings:
 *
 *   * reset period:  2400 (10 frames)
 *
 *   * alpha_d:       0.95 (represented to 9 d.p.)
 *
 *   * alpha_p:       0.2  (represented to 10 d.p.)
 *
 *   * alpha_s:       0.8  (represetned to 9 d.p.)
 *
 *   * delta:         1.5  
 *
 * \param[out] state       structure that holds the state. For each instance
 *                         of the noise suppressor a state must be declared.
 *                         this is then passed to the other functions to be
 *                         updated
 *
 * \param[in,out] sup      Suppressor state
 */
void sup_init_state(sup_state_t * state);


/** Function that suppresses residual noise in a frame
 *
 * 
 */
void sup_process_frame(sup_state_t * state,
                        int32_t output [SUP_FRAME_ADVANCE],
                        const int32_t input[SUP_FRAME_ADVANCE]);


#endif
