#include <stdio.h>
// #include "ic_api.h"


int test_frame(int in){
    return in * 2;
}

// int main(void){
//     ic_state_t ic_state;
//     ic_init(&ic_state);

//     int32_t y_data[IC_FRAME_ADVANCE];
//     int32_t x_data[IC_FRAME_ADVANCE];
//     int32_t output[IC_FRAME_ADVANCE];

//     ic_filter(&ic_state, y_data, x_data, output);
//     uint8_t vad = 0;
//     ic_adapt(&ic_state, output);

//     printf("Mu: %f\n", ldexp(ic_state.mu[0][0].mant, ic_state.mu[0][0].exp));
// }