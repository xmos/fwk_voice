// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef ADEC_DEFINES_H
#define ADEC_DEFINES_H

/**
 * @page page_adec_defines_h adec_defines.h
 * 
 * This header contains lib_adec public defines 
 *
 * @ingroup adec_header_file
 */
/**
 * @defgroup adec_defines   ADEC #define constants 
 */ 

/** 
 * @brief Number of frames far we look back to smooth the peak to average filter power ratio history
 * @ingroup adec_defines
 */
#define ADEC_PEAK_TO_AVERAGE_HISTORY_DEPTH         8

/**
 * @brief Number of frames of peak power history we look at while computing AEC goodness metric. NOT USER MODIFIABLE 
 * @ingroup adec_defines
 */
#define ADEC_PEAK_LINREG_HISTORY_SIZE           66

/**
 * @brief Initial delay of microphone in the DE mode in milliseconds.
 * This allows measuring up to `ADEC_DE_DELAY_MS` ms of delay in cases when the mic is earlier than the reference.
 * 
 * @ingroup adec_defines
 */
#define ADEC_DE_DELAY_MS                        150


/**
 * @brief Same as `ADEC_DE_DELAY_MS` but in samples.
 * 
 * @ingroup adec_defines
 */
#define ADEC_DE_DELAY_SAMPS                     (16000 * ADEC_DE_DELAY_MS / 1000)

#endif
