#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_input_state_init(vnr_input_state_t *input_state);
void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, const int32_t *new_x_frame);
void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);
void vnr_mel_compute(bfp_s32_t *squared_mag_spect, float_s64_t *filter_output);

//full functionality API
void vnr_extract_features(vnr_input_state_t *input_state, const int32_t *new_x_frame, /*for debug*/ bfp_complex_s32_t *fft_output, bfp_s32_t *squared_mag_out);
#endif
