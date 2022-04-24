#ifndef __VNR_FEATURES_API_H__
#define __VNR_FEATURES_API_H__

#include "vnr_features_state.h"

void vnr_input_state_init(vnr_input_state_t *input_state);
void vnr_form_input_frame(vnr_input_state_t *input_state, bfp_complex_s32_t *X, const int32_t *new_x_frame);
fixed_s32_t* vnr_make_slice(vnr_input_state_t *input_state, bfp_complex_s32_t *X);
void vnr_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice);
void vnr_normalise_patch(vnr_input_state_t *input_state, bfp_s32_t *normalised_patch);

#endif
