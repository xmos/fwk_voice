#include <stdio.h>
#include "ic_api.h"
#include "ic_low_level.h"
#include "calc_vnr_pred.h"

ic_state_t ic_state;
vnr_pred_state_t vnr_pred_state;

void test_init(void){
    ic_init(&ic_state);
    init_vnr_pred_state(&vnr_pred_state);
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

    calc_vnr_pred(&vnr_pred_state, &ic_state.Y_bfp[0], &ic_state.Error_bfp[0]);

    return vnr_pred_state.input_vnr_pred;
}

void test_set_ic_energies(double ie_s, double oe_s, double ie_f, double oe_f){
    //ic_state.ic_adaption_controller_state.input_energy_slow = double_to_float_s32(ie_s);
    //ic_state.ic_adaption_controller_state.output_energy_slow = double_to_float_s32(oe_s);
    //ic_state.ic_adaption_controller_state.input_energy_fast = double_to_float_s32(ie_f);
    //ic_state.ic_adaption_controller_state.output_energy_fast = double_to_float_s32(oe_f);
}
