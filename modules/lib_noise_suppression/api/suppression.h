// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef SUPPRESSION_H_
#define SUPPRESSION_H_


//#ifdef __suppression_conf_h_exists__
#include "suppression_conf.h"
//#endif

/*
 * SUP_DEBUG            Will enable all self checking, it will make it
 *                      MUCH slower however.
 * SUP_DEBUG_PRINT      Will enable printing of internal state to be
 *                      compared to higher models.
 * SUP_WARNING_PRINT    This enables warnings (events that might be bad
 *                      but not catastrophic).
 */
#ifndef SUP_DEBUG
#define SUP_DEBUG 0
#endif
#ifndef SUP_DEBUG_PRINT
#define SUP_DEBUG_PRINT 0
#endif
#ifndef SUP_WARNING_PRINT
#define SUP_WARNING_PRINT 0
#endif
#ifndef INT_EXP
#define INT_EXP -31
#endif

#ifndef SUP_X_CHANNELS
#error SUP_X_CHANNELS has not been set
#endif

#ifndef SUP_Y_CHANNELS
#error SUP_Y_CHANNELS has not been set
#endif

#ifndef SUP_FRAME_ADVANCE
#error SUP_FRAME_ADVANCE has not been set
#endif

#ifndef SUP_PROC_FRAME_LENGTH_LOG2
#error SUP_PROC_FRAME_LENGTH_LOG2 has not been set
#endif

#define SUP_SQRT_HANN_LUT           sqrt_hanning_480
#define SUP_OUTPUT_X_CHANNELS        (0)    //Enable outputting of X channels

#define SUP_PROC_FRAME_LENGTH      (1<<SUP_PROC_FRAME_LENGTH_LOG2)
#define SUP_PROC_FRAME_BINS_LOG2   (SUP_PROC_FRAME_LENGTH_LOG2 - 1)
#define SUP_PROC_FRAME_BINS        (1<<SUP_PROC_FRAME_BINS_LOG2)

#define SUP_INPUT_CHANNELS          (SUP_X_CHANNELS + SUP_Y_CHANNELS)
#define SUP_OUTPUT_CHANNELS         (SUP_OUTPUT_X_CHANNELS*SUP_X_CHANNELS + SUP_Y_CHANNELS)
#define SUP_OUTPUT_CHANNEL_PAIRS    ((SUP_OUTPUT_CHANNELS+1)/2)
#define SUP_INPUT_CHANNEL_PAIRS     ((SUP_INPUT_CHANNELS+1)/2)
#define SUP_Y_CHANNEL_PAIRS         ((SUP_Y_CHANNELS+1)/2)
#define SUP_X_CHANNEL_PAIRS         ((SUP_X_CHANNELS+1)/2)

#if  SUP_PROC_FRAME_LENGTH_LOG2 == 6
#define SUP_FFT_SINE_LUT dsp_sine_64
#define SUP_FFT_SINE_LUT_HALF dsp_sine_32
#elif  SUP_PROC_FRAME_LENGTH_LOG2 == 7
#define SUP_FFT_SINE_LUT dsp_sine_128
#define SUP_FFT_SINE_LUT_HALF dsp_sine_64
#elif  SUP_PROC_FRAME_LENGTH_LOG2 == 8
#define SUP_FFT_SINE_LUT dsp_sine_256
#define SUP_FFT_SINE_LUT_HALF dsp_sine_128
#elif  SUP_PROC_FRAME_LENGTH_LOG2 == 9
#define SUP_FFT_SINE_LUT dsp_sine_512
#define SUP_FFT_SINE_LUT_HALF dsp_sine_256
#elif  SUP_PROC_FRAME_LENGTH_LOG2 == 10
#define SUP_FFT_SINE_LUT dsp_sine_1024
#define SUP_FFT_SINE_LUT_HALF dsp_sine_512
#endif

/////////////////////////// noise suppression stuff ///////////////////////////

#include "sup_noise_state.h"
#include "suppression_state.h"

//void sup_test_task(chanend c_input, chanend c_output, chanend ?c_control);


void sup_dump_parameters(suppression_state_t * state);

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
void ns_init_state(ns_state_t * state);//////////////////////////////////////////////////////////////////////////

/** Function that sets the reset period in ms for a noise suppressor
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] ms           Reset period in milliseconds
 */
void sup_set_noise_reset_period_ms(suppression_state_t * state, int32_t ms); ///// NOT FOUND///

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
void sup_set_noise_alpha_d(suppression_state_t * state, int32_t alpha_d);/////////////////////////////////////////////////////////////

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
void sup_set_noise_alpha_s(suppression_state_t * state, int32_t alpha_s);////////////////////////////////////////////////////////////////

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
void sup_set_noise_alpha_p(suppression_state_t * state, int32_t alpha_p);////////////////////////////////////////////////////////////////

/** Function that sets the threshold for the noise suppressor to decide on
 * whether the current signal contains voice or not.
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] delta        The voice threshold represented in 8.24 format.
 */
void sup_set_noise_delta(suppression_state_t * state, int32_t delta); ////////////////////////////////////////////////////////////////

/** Function that sets the noise floor in db for a noise suppressor
 *
 * \param[in,out] state    Noise suppressor to be modified
 *
 * \param[in] db           Noise floor on a linear scale, i.e. 0.5 would be -6dB.
 */
void sup_set_noise_floor(suppression_state_t * state, int32_t noise_floor);////////////////////////////////////////////////////////////////////////////////

/** Function that resets the noise suppressor. Does not affect
 * any parameters that have been set, such as enable, alphas etc.
 *
 * \param[in,out] sup      Suppressor state, initialised
 */
void sup_reset_noise_suppression(suppression_state_t * sup);///////////////////////////////////////////////////////////////////////////////


/** Function that sets the enable flag of the noise suppressor.
 *
 * \param[in,out] sup      Suppressor state, initialised
 *
 * \param[in] enable       1 if suppresor should be enabled, 0 otherwise
 */
void sup_set_noise_suppression_enable(suppression_state_t * sup, int enable);/////////////////////////////////////////////////////////////


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


/** Function that resets the echo suppressor. Does not affect
 * any parameters that have been set, such as enable, mu etc.
 *
 * \param[in,out] sup      Suppressor state, initialised
 */
void sup_reset_echo_suppression(suppression_state_t * sup);

/** Function that suppresses residual echoes and noise in a frame
 *
 * 
 */
void sup_process_frame(bfp_s32_t * input, 
                        suppression_state_t * state, 
                        bfp_s32_t * output)

/** Function that sets the enable flag of the echo suppressor.
 *
 * \param[in,out] sup      Suppressor state, initialised
 *
 * \param[in] enable       1 if suppresor should be enabled, 0 otherwise
 */
void sup_set_echo_suppression_enable(suppression_state_t * sup, int enable);

/** Function that sets the maximum suppression of the echo suppressor.
 *
 * \param[in,out] sup      Suppressor state, initialised
 *
 * \param[in] db           Suppression in db - set to 0 to not suppress,
 *                         or to a negative value to suppress. Setting this
 *                         value beyond -10 causes artefacts
 */
void sup_set_echo_suppression_db(suppression_state_t * sup, int db);

/** Function that sets the learning rate of the echo suppressor
 *
 * \param[in,out] sup      Suppressor state, initialised
 *
 * \param[in] mu           learning rate.
 */
void sup_set_echo_suppression_mu(suppression_state_t * sup, int mu_24_8);

#endif
