// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef SUPPRESSION_H_
#define SUPPRESSION_H_

/////////////////////////// noise suppression stuff ///////////////////////////

//#include <suppression_conf.h>
#include "suppression_state.h"
//#include <suppression_window.h>

//#define SUP_SQRT_HANN_LUT           sqrt_hanning_480

//void sup_test_task(chanend c_input, chanend c_output, chanend ?c_control);


//void sup_dump_parameters(suppression_state_t * state);

/** Function that initialises the noise suppresion state.
 * It initialises the a noise suppressor with the following settings:
 *
 *   * reset period:  ?
 *
 *   * alpha_d:       0.95 (represented by 4080218931, 0.95* 0xFFFFFFFF)
 *
 *   * alpha_p:       0.2  (represented by 858993459,  0.2 * 0xFFFFFFFF)
 *
 *   * alpha_s:       0.8  (represetned by 3435973837, 0.8 * 0xFFFFFFFF)
 *
 *   * noise_floor: -40 dB
 *
 *   * delta:         5.0  (represented by 83886080,   5.0 * 0x01000000)
 *
 * \param[out] state       structure that holds the state. For each instance
 *                         of the noise suppressor a state must be declared.
 *                         this is then passed to the other functions to be
 *                         updated
 */


/** Function that sets the reset period in ms for a noise suppressor
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] ms           Reset period in milliseconds
 */
void sup_set_noise_reset_period_ms(suppression_state_t * state, int32_t ms);

/** Function that sets the alpha-value used in the recursive avarage of the
 * noise value. A fraction alpha is of the old value is combined with a
 * fraction (1-alpha) of the new value. This value is represented as an
 * unsigned 32-but integer between 0 (representing 0.0) and 0xFFFFFFFF
 * (representing 1.0)
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] alpha_d      Fraction, typically close to 1.0 (0xFFFFFFFF)
 */
void sup_set_noise_alpha_d(suppression_state_t * state, float_s32_t alpha_d);/////////////////////////////////////////////////////////////

/** Function that sets the alpha-value used in the recursive avarage ....
 * ........ A fraction alpha is of the old value is combined with a
 * fraction (1-alpha) of the new value. This value is represented as an
 * unsigned 32-but integer between 0 (representing 0.0) and 0xFFFFFFFF
 * (representing 1.0)
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] alpha_s      Fraction, typically close to 1.0 (0xFFFFFFFF)
 */
void sup_set_noise_alpha_s(suppression_state_t * state, float_s32_t alpha_s);////////////////////////////////////////////////////////////////

/** Function that sets the alpha-value used in the recursive avarage ....
 * ........ A fraction alpha is of the old value is combined with a
 * fraction (1-alpha) of the new value. This value is represented as an
 * unsigned 32-but integer between 0 (representing 0.0) and 0xFFFFFFFF
 * (representing 1.0)
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] alpha_p      Fraction, typically close to 1.0 (0xFFFFFFFF)
 */
void sup_set_noise_alpha_p(suppression_state_t * state, float_s32_t alpha_p);////////////////////////////////////////////////////////////////

/** Function that sets the threshold for the noise suppressor to decide on
 * whether the current signal contains voice or not.
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] delta        The voice threshold represented in 8.24 format.
 */
void sup_set_noise_delta(suppression_state_t * state, float_s32_t delta); ////////////////////////////////////////////////////////////////

/** Function that sets the noise floor in db for a noise suppressor
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] db           Noise floor on a linear scale, i.e. 0.5 would be -6dB.
 */
void sup_set_noise_floor(suppression_state_t * state, float_s32_t noise_floor);////////////////////////////////////////////////////////////////////////////////

/** Function that resets the noise suppressor. Does not affect
 * any parameters that have been set, such as enable, alphas etc.
 *
 * \param[in,out] sup      Suppressor state, initialised
 */
void sup_reset_noise_suppression(suppression_state_t * sup);///////////////////////////////////////////////////////////////////////////////



/** Function that initialises the suppression state.
 *
 * It initialises the a noise suppressor with the following settings:
 *
 *   * reset period:  ?
 *
 *   * alpha_d:       0.95 (represented by 4080218931, 0.95* 0xFFFFFFFF)
 *
 *   * alpha_p:       0.2  (represented by 858993459,  0.2 * 0xFFFFFFFF)
 *
 *   * alpha_s:       0.8  (represetned by 3435973837, 0.8 * 0xFFFFFFFF)
 *
 *   * noise_floor: -40 dB
 *
 *   * delta:         5.0  (represented by 83886080,   5.0 * 0x01000000)
 *
 * \param[out] state       structure that holds the state. For each instance
 *                         of the noise suppressor a state must be declared.
 *                         this is then passed to the other functions to be
 *                         updated
 *
 * \param[in,out] sup      Suppressor state
 */
void sup_init_state(suppression_state_t * state);


/** Function that suppresses residual noise in a frame
 *
 * 
 */
void sup_process_frame(suppression_state_t * state,
                        int32_t output [SUP_FRAME_ADVANCE],
                        const int32_t input[SUP_FRAME_ADVANCE]);


#endif
