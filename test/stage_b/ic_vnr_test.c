#include <stdio.h>
#include "ic_api.h"
#include "ic_low_level.h"
#include "vnr_features_api.h"
#include "vnr_inference_api.h"
#include "calc_vnr_pred.h"

ic_state_t ic_state;
vnr_feature_state_t vnr_feature_state;
float_s32_t input_vnr_pred = {0, 0};
fixed_s32_t pred_alpha = Q30(0.97);
vnr_pred_state_t vnr_pred_state;

int32_t test_init(void){
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init();
    ic_init(&ic_state);
    init_vnr_pred_state(&vnr_pred_state);
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

float_s32_t test_vnr(){

    bfp_s32_t feature_patch;
    int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
    vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &ic_state.Y_bfp[0]);

    float_s32_t inference_output;
    vnr_inference(&inference_output, &feature_patch);

    input_vnr_pred = float_s32_ema(input_vnr_pred, inference_output, pred_alpha);

    return input_vnr_pred;
}

void test_set_ic_energies(double ie_s, double oe_s, double ie_f, double oe_f){
    //ic_state.ic_adaption_controller_state.input_energy_slow = double_to_float_s32(ie_s);
    //ic_state.ic_adaption_controller_state.output_energy_slow = double_to_float_s32(oe_s);
    //ic_state.ic_adaption_controller_state.input_energy_fast = double_to_float_s32(ie_f);
    //ic_state.ic_adaption_controller_state.output_energy_fast = double_to_float_s32(oe_f);
}
