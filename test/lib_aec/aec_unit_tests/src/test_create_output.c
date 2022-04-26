// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

static const double WOLA_window_fp[32] = {
        0.0022640387134577056, 0.009035651368646647, 0.020253513192751316,
        0.03581603349196372, 0.05558227567253826, 0.0793732335844094,
        0.10697345262860625, 0.1381329809474649, 0.1725696330273574,
        0.20997154521440087, 0.24999999999999994, 0.2922924934990568,
        0.3364660183412892, 0.38212053224528636, 0.42884258086335736,
        0.4762090420881288, 0.5237909579118711, 0.5711574191366425,
        0.6178794677547135, 0.6635339816587108, 0.7077075065009433,
        0.7499999999999999, 0.790028454785599, 0.8274303669726425,
        0.861867019052535, 0.8930265473713938, 0.9206267664155905,
        0.9444177243274616, 0.9641839665080363, 0.9797464868072486,
        0.9909643486313533, 0.9977359612865423,};

void aec_calc_output_fp(double *output, double *error, double *overlap) {
    for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
        error[i] = 0.0;
    }
    //window error
    for(int i=0; i<32; i++) {
        error[AEC_FRAME_ADVANCE + i] = WOLA_window_fp[i] * error[AEC_FRAME_ADVANCE + i];
        error[AEC_PROC_FRAME_LENGTH - 32 + i] = WOLA_window_fp[32 - 1 - i] * error[AEC_PROC_FRAME_LENGTH - 32 + i];
    }
    //copy output to error
    for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
        output[i] = error[AEC_FRAME_ADVANCE + i];
    }
    //add previous frame overlap
    for(int i=0; i<32; i++) {
        output[i] += overlap[i];
    }
    //update overlap
    for(int i=0; i<32; i++) {
        overlap[i] = error[AEC_PROC_FRAME_LENGTH - 32 + i];
    }

    //normalise output to 1.31 and saturate
    double max = ldexp(0x7fffffff, -31);
    double min = ldexp(((double)(int32_t)0x80000000), -31);
    for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
        if(output[i] < min) {
            output[i] = min;
        }
        if(output[i] >= max) {
            output[i] = max;
        }
    }
}

static inline double abs_double(double a) {
    if(a < 0.0)
        return -a;
    else
        return a;
}

double max_diff = 0.0;
static inline void check_error(double ref, int32_t dut, int dut_exp, double rtol, double atol, int ch, int iter, const char *error_string) {
    double ref_double = ref;
    double dut_double = ldexp(dut, dut_exp);
    double diff = abs_double(ref_double - dut_double);
    if(diff > max_diff) diff = max_diff;
    if(diff > rtol*abs_double(ref_double) + atol)
    {
        printf("diff %.15f, %.15f\n",diff, rtol*ref_double + atol);
        printf("%s, iter %d, ch %d: diff %.15f, . ref %.15f, dut %.15f\n",error_string, iter, ch, diff, ref_double, dut_double);
        assert(0);
    }
}

void test_create_output() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 1;
    unsigned num_phases = AEC_MAIN_FILTER_PHASES - 1;

    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_state_t state, shadow_state;
    aec_shared_state_t aec_shared_state;
    aec_init(&state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, num_phases, num_phases);
    //Initialise floating point arrays
    double error_fp[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH];
    double output_fp[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
    double overlap_fp[AEC_MAX_Y_CHANNELS][32];
    for(int i=0; i<num_y_channels; i++) {
        for(int j=0; j<32; j++) {
            overlap_fp[i][j] = 0.0;
        }
    }
    unsigned seed = 2;
    //Generate error data
    for(unsigned iter=0; iter<(1<<12)/F; iter++) {
        int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
        aec_frame_init(&state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);
        int is_main_filter = pseudo_rand_uint32(&seed) % 2;
        aec_state_t *state_ptr;
        if(is_main_filter) {
            state_ptr = &state;
        }
        else {
            state_ptr = &shadow_state;
        }

        //state_ptr->error is initialised in the Error->error IFFT call. Initialise here for standalone testing
        for(int ch=0; ch<num_y_channels; ch++) {
            bfp_s32_init(&state_ptr->error[ch], (int32_t*)&state_ptr->Error[ch].data[0], 0, AEC_PROC_FRAME_LENGTH, 0);
        }

        for(int ch=0; ch<num_y_channels; ch++) {
            bfp_s32_t *error_ptr = &state_ptr->error[ch]; 
            error_ptr->exp = pseudo_rand_int(&seed, -31, 32);
            error_ptr->hr = (pseudo_rand_uint32(&seed) % 3);
            for(int i=0; i<AEC_PROC_FRAME_LENGTH; i++) {
                error_ptr->data[i] = pseudo_rand_int32(&seed) >> error_ptr->hr;
                error_fp[ch][i] = ldexp(error_ptr->data[i], error_ptr->exp);
            }
        }
        int32_t output[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
        for(int ch=0; ch<state_ptr->shared_state->num_y_channels; ch++) {
            aec_calc_output(state_ptr, &output[ch], ch);
        }

        for(int ch=0; ch<state_ptr->shared_state->num_y_channels; ch++) {
            aec_calc_output_fp(output_fp[ch], error_fp[ch], overlap_fp[ch]);
        }

        for(int ch=0; ch<state_ptr->shared_state->num_y_channels; ch++) {
            //check error
            bfp_s32_t *error_ptr = &state_ptr->error[ch]; 
            for(int i=0; i<AEC_PROC_FRAME_LENGTH; i++) {
                check_error(error_fp[ch][i], error_ptr->data[i], error_ptr->exp, 0.0000002, pow(10, -8), ch, iter, "error wrong");
            }

            //check output
            for(int i=0; i<AEC_FRAME_ADVANCE; i++) {
                check_error(output_fp[ch][i], output[ch][i], -31, 0.0000002, pow(10, -8), ch, iter, "error wrong");
            }

            //check overlap
            for(int i=0; i<32; i++) {
                check_error(overlap_fp[ch][i], state_ptr->overlap[ch].data[i], state_ptr->overlap[ch].exp, 0.0000002, pow(10, -8), ch, iter, "overlap wrong");
            }
        }
    }
    printf("max_diff = %.15f\n",max_diff);
}
