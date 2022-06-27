#include <stdio.h>
#include "ic_api.h"
#include "ic_low_level.h"
#include "vnr_features_api.h"
#include "vnr_inference_api.h"

ic_state_t ic_state;
vnr_input_state_t vnr_input_state;
vnr_feature_state_t vnr_feature_state;

int32_t test_init(void){
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init();
    ic_init(&ic_state);
    //Custom setup for testing
    ic_state.ic_adaption_controller_state.adaption_controller_config.enable_adaption_controller = 1;
    // ic_state.config_params.delta = double_to_float_s32(0.0156);

    return err;
}

ic_state_t test_get_ic_state(void){
    return ic_state;
}

void test_filter(
        int32_t y_data[IC_FRAME_ADVANCE],
        int32_t x_data[IC_FRAME_ADVANCE],
        int32_t output[IC_FRAME_ADVANCE]){
    ic_filter(&ic_state, y_data, x_data, output);
}

void test_adapt(
        float_s32_t vnr,
        int32_t output[IC_FRAME_ADVANCE]){
    ic_adapt(&ic_state, vnr, output);
}

float_s32_t test_vnr(
        const int32_t input[VNR_FRAME_ADVANCE]){
    complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
    bfp_complex_s32_t X;
    vnr_form_input_frame(&vnr_input_state, &X, input_frame, input);

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

    float_s32_t inference_output;
    vnr_inference(&inference_output, &feature_patch);

    return inference_output;
}

void test_set_ic_energies(double ie_s, double oe_s, double ie_f, double oe_f){
    ic_state.ic_adaption_controller_state.input_energy_slow = double_to_float_s32(ie_s);
    ic_state.ic_adaption_controller_state.output_energy_slow = double_to_float_s32(oe_s);
    ic_state.ic_adaption_controller_state.input_energy_fast = double_to_float_s32(ie_f);
    ic_state.ic_adaption_controller_state.output_energy_fast = double_to_float_s32(oe_f);
}