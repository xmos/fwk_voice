#include <stdio.h>
#include "ic_api.h"
#include "ic_low_level.h"

ic_state_t ic_state;

int test_init(void){
    int ret = ic_init(&ic_state);
    return ret;
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

void test_adapt(float_s32_t vnr){
    ic_adapt(&ic_state, vnr);
}

float_s32_t test_vnr(){

    float_s32_t input_vnr_pred, output_vnr_pred;
    ic_calc_vnr_pred(&ic_state, &input_vnr_pred, &output_vnr_pred);

    return input_vnr_pred;
}

void test_control_system(double vnr_fl, int32_t ad_config, double fast_ratio){
    ic_state.ic_adaption_controller_state.adaption_controller_config.adaption_config = ad_config;
    ic_state.ic_adaption_controller_state.fast_ratio = f64_to_float_s32(fast_ratio);
    float_s32_t vnr = f64_to_float_s32(vnr_fl);
    ic_mu_control_system(&ic_state, vnr);
}
