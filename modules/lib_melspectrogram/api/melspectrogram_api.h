// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _MEL_SPECTROGRAM_H
#define _MEL_SPECTROGRAM_H

#include <stdint.h>
#include <stdbool.h>
#include "xmath/xmath.h"
#include "mel_spec_settings.h"
#ifdef __XC__
    #define _Bool int
#endif

typedef enum
{
  MEL_SPEC_SMALL,
  MEL_SPEC_LARGE
} mel_spec_option_t;

/* @brief Generates a Mel spectrogram from an input vector.
 *
 * This function very specifically supports only two modes of operation, termed
 * "small" and "large".
 *
 * In "small" operation, the function expects @p input to be of form int16_t[6400]
 * and @p output to be of the form int8_t[1][64][26][4]. It will process the input
 * vector with a 512-width STFT with a 256-wide hop distance and will use a
 * 512-width Hanning window. It will then create a 64-frame Mel spectrogram from 
 * this data. It will optionally convert the output to dB, but
 * first setting all values below 1e-10 to 1e-10; after this conversion, it will
 * also set all values that are less than the maximum value in the output minus
 * 80 to max(output) - 80. It will then optionally subtract the mean of the
 * output from every output value.
 * It will then optionally quantise using the formula
 * int8_t output[] = 0.3325621485710144 * (output + 12) for all
 * output, before finally arranging data in the output buffer. The first and
 * last dimension indices will be assumed to be 0.
 * Therefore, output[0][*][*][0] will always contain data. The rest of the array
 * will be 0.
 * When using "small" operation, the parameters @p out_trim_top and 
 * @p out_trim_end **MUST** be NULL. Passing non-NULL pointers to these inputs
 * in this mode will lead to untested behaviour, unless the _TRIM_START and 
 * _TRIM_END macros are also redefined in mel_spec_settings.h
 * Set this by passing MEL_SPEC_SMALL to @p mel_spec_option.
 *
 * In "large" operation, the function expects @p input to be of the form
 * int16_t[84800] and @p output to be of the form int8_t[1][128][158][4]. It will
 * process the input vector with a 1024-width STFT with a 512-wide hop distance
 * and will use a 1024-width Hanning window. It will then create a 166-frame Mel 
 * spectrogram from this data - note that this is greater than the 158 output. 
 * It will optionally convert the output to dB. It will first add 1e-8 to all 
 * values. After this, it will then set all values below 1e-10 to 1e-10; 
 * after this conversion, it will also set all values that are less than the 
 * maximum value in the output minus 80 to max(output) - 80. 
 * It will then optionally subtract the mean of the output from every output 
 * value. It will then optionally quantise using the formula 
 * int8_t output[] = 0.003921553026884794 * (output + 128) for all output, 
 * before finally arranging data in the output buffer. The first and
 * last dimension indices will be assumed to be 0.
 * Therefore, output[0][*][*][0] will always contain data. The rest of the array
 * will be 0.
 * As noted above, the Mel spectrogram produced in this process is 166-wide, but
 * the output vector is only 158 wide. The remaining 8 frames (4 from the start,
 * and 4 from the end) will be placed in @p out_trim_top and @p out_trim_end
 * with the same processing as above.
 * Set this by passing MEL_SPEC_LARGE to @p mel_spec_option.
 *
 * The above hard-coded values can be altered by editing mel_spec_settings.h
 *
 * This function is not thread-safe.
 * /p output is edited in place, and is not safe to access during operation
 * /p input is accessed in place, and is not safe to access during operation
 *
 * Performance statistics:
 *                                        Small |    Large
 * -------------------------------------------------------
 * Memory usage inc. IO buffers, bytes: ~ 59628 | ~ 294796
 *        Time to return, milliseconds: ~ 24.13 | ~ 306.75
 *
 * @param[out] output - Output. Takes a pointer to int8_t[1][n_mels][n_frames][4],
 *                   where n_mels is either 64 or 128 and
 *                   n_frames is either 26 or 158.
 * @param[out] out_trim_top - If trim settings are applied, the first TRIM_START
 *                            frames of output are placed into this buffer. May
 *                            be either int8_t[1][n_mels][TRIM_START][4] or NULL
 * @param[out] out_trim_end - If trim settings are applied, the last TRIM_END
 *                            frames of output are placed into this buffer. May
 *                            be either int8_t[1][n_mels][TRIM_END][4] or NULL
 * @param[in] input   - Input vector. Takes a pointer to int16_t[n_samples], where
 *                   n_samples is either 6400 or 84800.
 * @param[in] mel_spec_option - Chooses between "small" operation with
 *                              MEL_SPEC_SMALL or "large" operation with
 *                              MEL_SPEC_LARGE.
 * @param[in] quantise - Chooses whether transform with scale and zero-point
 * @param[in] convert_to_db - Chooses whether to convert to dB before mean
 *                            normalisation and quantisation
 * @param[in] subtract_mean - Chooses whether to subtract the mean of the output
 *                            tensor before quantisation
 */
void x_melspectrogram(int8_t *output,
                      int8_t *out_trim_top,
                      int8_t *out_trim_end,
                      int16_t *input,
                      mel_spec_option_t mel_spec_option,
                      bool quantise,
                      bool convert_to_db,
                      bool subtract_mean);

#ifdef __XC__
    #undef _Bool
#endif

#endif // _MEL_SPECTROGRAM_H
