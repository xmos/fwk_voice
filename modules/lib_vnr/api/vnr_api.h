#ifndef __VNR_API_H__
#define __VNR_API_H__

#include "vnr_state.h"

void vnr_inference_init(vnr_ie_t *ie_ptr);
void vnr_inference(vnr_ie_t *ie_ptr, int8_t *vnr_output);
void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, int32_t *new_x_frame);
void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data);

//full functionality API
void vnr_extract_features(int32_t *new_x_frame);
#endif
