// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_DEFINES_H
#define AEC_DEFINES_H

/**
 * @page page_aec_defines_h aec_defines.h
 * 
 * This header contains lib_aec public defines 
 *
 * @ingroup aec_header_file
 */
/**
 * @defgroup aec_defines   AEC #define constants
 */ 

/** @brief Maximum number of microphone input channels supported in the library.
 * Microphone input to the AEC refers to the input from the device's microphones from which AEC removes the echo
 * created in the room by the device's loudspeakers.
 *
 * AEC functions follow the convention of using @math{y} and @math{Y} for referring to time domain and frequency domain
 * representation of microphone input.
 *
 * The `num_y_channels` passed into aec_init() call should be less than or equal to AEC_LIB_MAX_Y_CHANNELS.
 * This define is only used for defining data structures in the aec_state. The library code implementation uses only the
 * num_y_channels aec is initialised for in the aec_init() call.  
 *
 * @ingroup aec_defines
 */
#define AEC_LIB_MAX_Y_CHANNELS (2)

/** @brief Maximum number of reference input channels supported in the library.
 * Reference input to the AEC refers to a copy of the device's speaker output audio that is also sent as an input to the
 * AEC. It is used to model the echo characteristics between a mic-loudspeaker pair.
 *
 * AEC functions follow the convention of using @math{x} and @math{X} for referring to time domain and frequency domain
 * representation of reference input.
 *
 * The `num_x_channels` passed into aec_init() call should be less than or equal to AEC_LIB_MAX_X_CHANNELS.
 * This define is only used for defining data structures in the aec_state. The library code implementation uses only the
 * num_x_channels aec is initialised for in the aec_init() call.  
 *
 * @ingroup aec_defines
 */
#define AEC_LIB_MAX_X_CHANNELS (2)

/** @brief AEC frame size
 * This is the number of samples of new data that the AEC works on every frame. 240 samples at 16kHz is 15msec. Every
 * frame, the echo canceller takes in 15msec of mic and reference data and generates 15msec of echo cancelled output.
 *
 * @ingroup aec_defines
 */
#define AEC_FRAME_ADVANCE (240)

/** Time domain samples block length used internally in AEC's block LMS algorithm
 *
 * @ingroup aec_defines
 */
#define AEC_PROC_FRAME_LENGTH (512)

/** Number of bins of spectrum data computed when doing a DFT of a AEC_PROC_FRAME_LENGTH length time domain vector. The
 * AEC_FD_FRAME_LENGTH spectrum values represent the bins from DC to Nyquist.
 *
 * @ingroup aec_defines
 */   
#define AEC_FD_FRAME_LENGTH ((AEC_PROC_FRAME_LENGTH / 2) + 1)

/** @brief Maximum total number of phases supported in the AEC library 
 * This is the maximum number of total phases supported in the AEC library. Total phases are calculated by summing
 * phases across adaptive filters for all x-y pairs.
 *
 * For example. for a 2 y-channels, 2 x-channels, 10 phases per x channel configuration, there are 4 adaptive filters,
 * H_hat<SUB>y0x0</SUB>, H_hat<SUB>y0x1</SUB>, H_hat<SUB>y1x0</SUB> and H_hat<SUB>y1x1</SUB>, each filter having 10
 * phases, so the total number of phases is 40.
 * When aec_init() is called to initialise the AEC, the num_y_channels, num_x_channels and num_main_filter_phases
 * parameters passed in should be such that num_y_channels * num_x_channels * num_main_filter_phases is less than equal
 * to AEC_LIB_MAX_PHASES. 
 *
 * This define is only used when defining data structures within the AEC state structure. The AEC algorithm
 * implementation uses the num_main_filter_phases and num_shadow_filter_phases values that are passed into aec_init().
 *
 * @ingroup aec_defines
 */
#define AEC_LIB_MAX_PHASES (AEC_LIB_MAX_Y_CHANNELS * AEC_LIB_MAX_X_CHANNELS * 10)

/** Overlap data length
 *
 * @ingroup aec_defines
 */
#define AEC_UNUSED_TAPS_PER_PHASE (16)

/** Extra 2 samples you need to allocate in time domain so that the full spectrum (DC to nyquist) can be stored
 * after the in-place FFT. NOT USER MODIFIABLE.
 *
 * @ingroup aec_defines
 */  
//
#define AEC_FFT_PADDING (2)

#define AEC_ZEROVAL_EXP (-1024) /// A very small exponent indicating 0 value.
#define AEC_ZEROVAL_HR (31) /// Headroom value used in BFP arrays when indicating 0 value by setting exponent to AEC_ZEROVAL_EXP 

#endif
