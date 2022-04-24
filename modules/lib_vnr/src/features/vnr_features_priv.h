#ifndef __VNR_PRIV_H__
#define __VNR_PRIV_H__

#include "vnr_features_state.h"

void vnr_priv_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);
void vnr_priv_mel_compute(float_s32_t *filter_output, bfp_complex_s32_t *X);
void vnr_priv_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length);
// Private functions
fixed_s32_t vnr_priv_float_s32_to_fixed_q24_log2(float_s32_t x);

#endif
