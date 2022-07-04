
#include "calc_vnr_pred.h"
#include "q_format.h"

void init_vnr_pred_state(vnr_pred_state_t *vnr_pred_state)
{
    // Initialise vnr_pred_state
    vnr_feature_state_init(&vnr_pred_state->feature_state[0]);
    vnr_feature_state_init(&vnr_pred_state->feature_state[1]);
    
    vnr_inference_init();
    vnr_pred_state->pred_alpha_q30 = Q30(0.97);
    vnr_pred_state->input_vnr_pred = (float_s32_t){0, 0};
    vnr_pred_state->output_vnr_pred = (float_s32_t){0, 0};    
}

void calc_vnr_pred(
    vnr_pred_state_t *vnr_state,
    const bfp_complex_s32_t *Y,
    const bfp_complex_s32_t *Error
    )
{
    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH * VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_state->feature_state[0], &feature_patch, feature_patch_data, Y);
    float_s32_t ie_output;
    vnr_inference(&ie_output, &feature_patch);
    vnr_state->input_vnr_pred = float_s32_ema(vnr_state->input_vnr_pred, ie_output, vnr_state->pred_alpha_q30); 
    
    vnr_extract_features(&vnr_state->feature_state[1], &feature_patch, feature_patch_data, Error);
    vnr_inference(&ie_output, &feature_patch);
    vnr_state->output_vnr_pred = float_s32_ema(vnr_state->output_vnr_pred, ie_output, vnr_state->pred_alpha_q30);
}


