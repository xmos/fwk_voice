
#include "aec_config.h"
#include "aec_api.h"

float_s32_t aec_calc_max_input_energy_c_wrapper(int32_t (*input)[AEC_FRAME_ADVANCE], int channels) {
    //To be able to pass input in a function expecting pointer to const data
    float_s32_t temp = aec_calc_max_input_energy(input, channels);
    return temp;
}
