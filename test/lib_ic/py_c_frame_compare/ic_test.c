#include <stdio.h>
#include "ic_api.h"

ic_state_t ic_state;

void test_init(void){
    ic_init(&ic_state);
    //Custom setup for testing
    ic_state.ic_adaption_controller_state.adaption_controller_config.adaption_config = IC_ADAPTION_FORCE_ON;
}

ic_state_t test_get_state(void){

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
