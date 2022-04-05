#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, int32_t *new_x_frame);
void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);

//full functionality API
void vnr_extract_features(int32_t *new_x_frame);
#endif
