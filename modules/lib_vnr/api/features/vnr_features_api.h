#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_input_state_init(vnr_input_state_t *input_state);
void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, const int32_t *new_x_frame);
void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);
void vnr_mel_compute(float_s32_t *filter_output, bfp_complex_s32_t *X);
void vnr_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length);
void vnr_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice);
void vnr_normalise_patch(bfp_s32_t *normalised_patch, const fixed_s32_t (*feature_patch)[VNR_MEL_FILTERS]);
void vnr_quantise_patch(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_features_config_t *features_config);
void vnr_dequantise_output(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_features_config_t *features_config);

// Private functions
fixed_s32_t float_s32_to_fixed_q24_log2(float_s32_t x);

//full functionality API
void vnr_extract_features(int8_t* quantised_patch, vnr_input_state_t *input_state, const int32_t *new_x_frame);
#endif
