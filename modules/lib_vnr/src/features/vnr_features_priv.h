#ifndef __VNR_PRIV_H__
#define __VNR_PRIV_H__

#include "vnr_features_state.h"

/** Exponent of VNR input data. 
 * NOT USER MODIFIABLE.
 */
#define VNR_INPUT_EXP (-31)

/** Extra 2 samples you need to allocate in time domain so that the full spectrum (DC to nyquist) can be stored
 * after the in-place FFT. NOT USER MODIFIABLE.
 */  
#define VNR_FFT_PADDING (2)

/** Exponent of output of the log2 function used in the VNR
 * @ingroup vnr_features_state
 */
#define VNR_LOG2_OUTPUT_EXP (-24)
   
// Matching with python names
void vnr_priv_make_slice(fixed_s32_t *new_slice, const bfp_complex_s32_t *X);
void vnr_priv_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice);
void vnr_priv_normalise_patch(bfp_s32_t *normalised_patch, int32_t *normalised_patch_data, const vnr_feature_state_t *feature_state);

void vnr_priv_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);
void vnr_priv_mel_compute(float_s32_t *filter_output, const bfp_complex_s32_t *X);
void vnr_priv_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length);
fixed_s32_t vnr_priv_float_s32_to_fixed_q24_log2(float_s32_t x);

#endif
