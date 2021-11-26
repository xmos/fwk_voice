// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
extern "C"{
    #include "aec_defines.h"
    #include "aec_api.h"
}


/*
typedef struct {
    bfp_complex_s32_t Y_hat[AEC_LIB_MAX_Y_CHANNELS];
    bfp_complex_s32_t Error[AEC_LIB_MAX_Y_CHANNELS];
    bfp_complex_s32_t H_hat_1d[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_X_CHANNELS*AEC_LIB_MAX_PHASES]; 
    bfp_complex_s32_t X_fifo_1d[AEC_LIB_MAX_X_CHANNELS*AEC_LIB_MAX_PHASES];
    bfp_complex_s32_t T[AEC_LIB_MAX_X_CHANNELS];

    bfp_s32_t inv_X_energy[AEC_LIB_MAX_X_CHANNELS];
    bfp_s32_t X_energy[AEC_LIB_MAX_X_CHANNELS];
    bfp_s32_t output[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t overlap[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t y_hat[AEC_LIB_MAX_Y_CHANNELS];
    bfp_s32_t error[AEC_LIB_MAX_Y_CHANNELS];

    float_s32_t mu[AEC_LIB_MAX_Y_CHANNELS][AEC_LIB_MAX_X_CHANNELS];
    float_s32_t error_ema_energy[AEC_LIB_MAX_Y_CHANNELS];
    float_s32_t overall_Error[AEC_LIB_MAX_Y_CHANNELS]; 
    float_s32_t max_X_energy[AEC_LIB_MAX_X_CHANNELS]; 
    float_s32_t delta_scale;
    float_s32_t delta;

    aec_shared_state_t *shared_state; //pointer to the state shared between shadow and main filter
    unsigned num_phases;
}aec_state_t;
*/

#undef DWORD_ALIGNED
#define DWORD_ALIGNED [[aligned(8)]]

#define TEST_LEN (AEC_PROC_FRAME_LENGTH/2 + 1)
void test_delay_estimate() {
    unsafe {

    uint8_t DWORD_ALIGNED aec_memory_pool[sizeof(aec_memory_pool_t)];
    aec_state_t DWORD_ALIGNED state;
    aec_shared_state_t DWORD_ALIGNED shared_state;

    const unsigned num_phases = 30;
    unsigned seed = 34575;

    unsigned ch = 0;

    //Populate selected phase with energy to see if we can read peak
    for(unsigned ph = 0; ph < num_phases; ph++){
        aec_init(&state, NULL, &shared_state, aec_memory_pool, NULL, 1, 1, num_phases, 0);

        unsigned length = state.H_hat_1d[ch][ph].length;
        // printf("length: %d\n", length);

        for(unsigned i = 0; i < length; i++){
            state.H_hat_1d[ch][ph].data[i].re = att_random_uint32(seed);
            state.H_hat_1d[ch][ph].data[i].im = att_random_uint32(seed);
            state.H_hat_1d[ch][ph].exp = -10;
        }
        int measured_delay = aec_estimate_delay(&state);
        int actual_delay = ph * AEC_FRAME_ADVANCE;
        // printf("test_delay_estimate: %d (%d), fin\n", measured_delay, actual_delay);
        TEST_ASSERT_EQUAL_INT32_MESSAGE(measured_delay, actual_delay, "Delay estimate incorrect");
    }
    
    // unsigned seed = 34575;
    // for(int iter = 0; iter<(1<<12)/F; iter++) {
    //     dut_in.exp = sext(att_random_int32(seed), 6);        
    //     dut_in.hr = att_random_uint32(seed) % 4;
    //     for(int i=0; i<TEST_LEN; i++) {
    //         dut_in.data[i].re = att_random_int32(seed) >> dut_in.hr;
    //         dut_in.data[i].im = att_random_int32(seed) >> dut_in.hr;

    //     }
    //     double ref_out;
    //     calc_fd_frame_energy_fp(&ref_out, ref_in, TEST_LEN);
    //     float_s32_t dut_out;
    //     aec_calc_fd_frame_energy(&dut_out, &dut_in); //this only works for input size AEC_PROC_FRAME_LENGTH/2 + 1 since there is a static allocation of scratch memory of this size within the function

    //     //printf("ref %f, dut %f\n",ref_out, att_int32_to_double(dut_out.mant, dut_out.exp));
    //     int32_t ref = att_double_to_int32(ref_out, dut_out.exp);
    //     int32_t dut = dut_out.mant;
    //     TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<2, ref, dut, "Output delta is too large");
    // }
    }
}
