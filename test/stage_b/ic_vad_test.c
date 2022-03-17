#include <stdio.h>
#include "ic_api.h"
#include "vad_api.h"

ic_state_t ic_state;
vad_state_t vad_state;

void test_init(void){
    vad_init(&vad_state);
    ic_init(&ic_state);
    //Custom setup for testing
    ic_state.ic_adaption_controller_state.adaption_controller_config.enable_adaption_controller = 1;
    // ic_state.config_params.delta = double_to_float_s32(0.0156);

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
        uint8_t vad,
        int32_t output[IC_FRAME_ADVANCE]){
    ic_adapt(&ic_state, vad, output);
}

uint8_t test_vad(
        const int32_t input[VAD_FRAME_ADVANCE]){
    uint8_t vad = vad_probability_voice(input, &vad_state);
    return vad;
}