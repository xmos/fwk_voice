// Copyright (c) 2019-2020, XMOS Ltd, All rights reserved
#ifndef _VTB_UTILITIES_H_
#define _VTB_UTILITIES_H_

#include <stdint.h>
#include "dsp.h"
#include "vtb_q_format.h"
#include "vtb_float.h"
#include "vtb_ch_pair.h"


/**
 * Applies a "periodic(DFT-Even)" Hanning window function to a complex array
 * where the DC and NQ are stored in the real and imag components of bin zero
 * respectivly.
 *
 * \param[in]   d               complex array.
 *
 * \param[in]   length          Length of input array.
 */
void vtb_fd_hanning(dsp_complex_t * d, unsigned length);

/**
 * Fills the unsigned array abs_d with the element wise absolute values of the
 * complex array d. The exponent of abs_d is one more than the exponent of d.
 * The zero'th element is special as it implicitly represents the DC and NQ.
 * The abs of the DC is stored at abs_d[0]. The NQ is dropped.
 *
 * \param[in]   abs_d           32 bit unsigned array.
 *
 * \param[in]   d               complex array.
 *
 * \param[in]   length          Length of input array.
 */
void vtb_abs_channel(uint32_t * abs_d,
                     dsp_complex_t * d,
                     unsigned length);

/**
 * Returns the minimum number of leading zeros for an unsigned 32 bit array.
 *
 * \param[in]   arr             32 bit unsigned array.
 *
 * \param[in]   length          Length of input array.
 *
 * \returns                     Minimum leading zeros for elements in array.
 */
unsigned vtb_clz_uint32(uint32_t *arr, unsigned length);


/**
 * Counts leading zeros of a 64 bit unsigned int.
 *
 * \param[in]   x               64 bit unsigned int.
 */
unsigned vtb_clz64(uint64_t x);


/**
 * Calculates the power of a frequency domain frame.
 * Returns the power as a mantissa exponent pair.
 *
 * \param[in]       X           Array of freq domain samples.
 *
 * \param[in]       X_exp       Block floating-point exponent of X.
 *
 * \param[in]       length      Length of X.
 *
 * \returns                     Frame power
 */
vtb_u32_float_t vtb_get_fd_frame_power(dsp_complex_t * X,
        int X_exp,
        unsigned length);


/**
 * Calculates the power of a time domain frame.
 * Returns the power as a mantissa exponent pair.
 *
 * \param[in]       x               Array of time domain samples.
 *
 * \param[in]       x_exp           Block floating-point exponent of x.
 *
 * \param[in]       length          Length of x.
 *
 * \param[in]       select_ch_b     Calculate frame power of ch_b if 1. ch_a if 0.
 *
 * \returns                         Frame power
 */
vtb_u32_float_t vtb_get_td_frame_power(vtb_ch_pair_t * x,
        int x_exp,
        unsigned length,
        int select_ch_b);


/**
 * Return true if channel contains sample greater than epsilon.
 *
 * \param[in]       data            Array of time domain samples.
 *
 * \param[in]       length          Length of x.
 *
 * \param[in]       select_ch_b     Check samples of ch_b if 1, ch_a if 0.
 *
 * \param[in]       epsilon         Threshold value for activity.
 *
 * \returns                         Frame power
 */
int32_t vtb_is_frame_active(vtb_ch_pair_t * data,
        unsigned length,
        int select_ch_b,
        uint32_t epsilon);


/**
 * Function that returns the minimum leading sign bits for a channel in an array
 * of time domain samples.
 *
 * \param[in]       data            Array of time domain samples.
 *
 * \param[in]       length          Length of data array.
 *
 * \param[in]       select_ch_b     Select ch_b in data if 1. Selects ch_a if 0.
 *
 * \returns                         Minimum leading sign bits in selected channel.
 */
unsigned vtb_get_channel_hr(vtb_ch_pair_t * data,
        unsigned length,
        int select_ch_b);

#ifdef __XC__
/**
 * Function that returns the minimum leading sign bits for both channels
 * in an array of time domain samples.
 *
 * \param[in]       data            Array of time domain samples.
 *
 * \param[in]       length          Length of sample array.
 *
 * \returns                         Minimum leading sign bits for both channels.
 */
{unsigned, unsigned} vtb_get_channel_pair_hr(vtb_ch_pair_t * data,
        unsigned length);
#endif // __XC__


/**
 * Takes mu multiplies it by mu_scalar then limits the results to being between
 * mu_high and mu_low. Note, applies high limit first then low.
 *
 * \param[in]       mu              Value to be scaled and limited.
 *
 * \param[in]       mu_high         Upper bound on mu.
 *
 * \param[in]       mu_low          Lower bound on mu.
 *
 * \param[in]       mu_scalar       Scalar multiplication factor.
 *
 * \returns                          Scaled and limited mu value.
 */
vtb_uq1_31_t vtb_scale_and_limit_mu(vtb_uq1_31_t mu,
        vtb_uq1_31_t mu_high,
        vtb_uq1_31_t mu_low,
        vtb_uq8_24_t mu_scalar);


/**
 * Copies one channel (real or imag) of input_pair into output_pair for
 * specified number of elements.
 *
 * \param[out]      output_pair         Output array.
 *
 * \param[in]       input_pair          Input array.
 *
 * \param[in]       count               Number of elements to be copied.
 *
 * \param[in]       input_ch_b          Copy from ch_b of input array.
 *
 * \param[in]       ouput_ch_b          Copy to ch_b of output array.
 */
void vtb_save_input_to_output(vtb_ch_pair_t * output_pair,
        vtb_ch_pair_t * input_pair,
        unsigned count,
        int input_ch_b,
        int ouput_ch_b);

#ifdef __XC__
/**
 * Copies a pair of input channels to the output buffer. This is intended
 * to be used on time domain buffers. It saves the output for the simga
 * calculation that is later on.
 * Note, the exponents are all 0 as they are all on the raw input values.
 *
 * \param[out]      output_pair         Output array.
 *
 * \param[in]       input_pair          Input array.
 *
 * \param[in]       count               Number of elements to be copied.
 */
void vtb_save_input_to_output_pair(vtb_ch_pair_t * unsafe output_pair,
        vtb_ch_pair_t * unsafe input_pair,
        unsigned count);
#endif

/**
 * This applies a complex steering vector to the data array, scales it by scalar
 * and can shift left.
 *
 * The equlivent operation in python is to multiply the input vector by
 * scalar * np.exp(2.0j*samples*np.pi*np.arange(len(data))/M) * 2.0**shift_left
 * where M is the length of data when it was in the time domain
 *
 * \param[in]      samples          Number of samples to delay data by.
 *
 * \param[in]      scalar           Scaling factor on output.
 *
 * \param[in,out]  data             Frequency domain input.
 *
 * \param[in]      length           Length of data.
 *
 * \param[in]      shift_left       Left-shift amount.
 */
void vtb_steer(vtb_q7_24_t samples,
        vtb_q7_24_t scalar,
        dsp_complex_t data[],
        unsigned length,
        int shift_left);



#endif /* _VTB_UTILITIES_H_ */
