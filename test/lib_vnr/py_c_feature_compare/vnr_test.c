#include <stdio.h>
#include "vnr_features_api.h"
#include "vnr_inference_api.h" 

vnr_input_state_t vnr_input_state;
vnr_feature_state_t vnr_feature_state;

int test_init(void){
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int err = vnr_inference_init();
    return err;
}

vnr_feature_state_t test_get_feature_state(void){
    return vnr_feature_state;
}

void test_vnr_features(
    bfp_s32_t *feature_bfp,
    int32_t *feature_data,
    const int32_t *new_x_frame
    )
{
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;

    vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_x_frame);
    vnr_extract_features(&vnr_feature_state, feature_bfp, feature_data, &X);
}

double test_vnr_inference(
    bfp_s32_t *features
    )
{
    float_s32_t ie_output;
    vnr_inference(&ie_output, features);
    double result = float_s32_to_double(ie_output);
    return result;
}
