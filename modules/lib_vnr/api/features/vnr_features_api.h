#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_input_state_init(vnr_input_state_t *input_state);
void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, const int32_t *new_x_frame);
void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);
void vnr_mel_compute(float_s32_t *filter_output, bfp_complex_s32_t *X);
void vnr_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length);

// Private functions
fixed_s32_t float_s32_to_fixed_q24_log2(float_s32_t x);

//full functionality API
void vnr_extract_features(vnr_input_state_t *input_state, const int32_t *new_x_frame, /*for debug*/ bfp_complex_s32_t *fft_output, bfp_s32_t *squared_mag_out);
#endif
