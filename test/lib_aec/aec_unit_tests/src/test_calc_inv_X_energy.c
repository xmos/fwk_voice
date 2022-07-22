// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)
//right shift with symmetry around the first element (DC)
static void shift_mirror_posi(double *output, const double *input, int shift, int length) {
    //eg. [1, 2, 3, 4, 5] after shift of 2 becomes [3, 2, 1(DC), 2, 3]

    //first copy the shifted part
    for(int i=0; i<length-shift; i++) {
        output[shift+i] = input[i]; //[-, -, 1, 2, 3]
    }
    //DC (1st element in the original array) is now at index 'shift'
    int sym_index = shift;
    //mirror around sym_index
    for(int i=0; i<shift; i++) {
        output[sym_index - 1 - i] = output[sym_index + 1 + i]; //[3, 2, -, -, -]
    }
}

//left shift with symmetry around the last element (nyquist).
static void shift_mirror_negi(double *output, const double *input, int shift, int length) {
    shift = -shift;
    //eg. [1, 2, 3, 4, 5] after shift of 2 becomes [3, 4, 5(nyquist), 4, 3]  
    //first copy the shifted part => [3, 4, 5, -, -]
    for(int i=0; i<length-shift; i++) {
        output[i] = input[shift+i];
    }
    //The last element in the original array is now at index length-shift-1
    //Symmetry exists around this element
    int sym_index = length - shift - 1;
    //now do symmetry around sym_index and populate the remaining length
    //In our eg. this will populate [-, -, -, 4, 3]
    for(int i=0; i<shift; i++) {
        output[sym_index +1 + i] = output[sym_index -1 - i]; 
    }
}

static void vect_smooth(double *output, double *scratch, const double *norm_denom, const double *taps, int input_length) {
    for(int i=0; i<input_length; i++) {
        output[i] = norm_denom[i]*taps[2];
    }
    shift_mirror_negi(scratch, norm_denom, -2, input_length);
    for(int i=0; i<input_length; i++) {
        output[i] += scratch[i]*taps[0];
    }
    shift_mirror_negi(scratch, norm_denom, -1, input_length);
    for(int i=0; i<input_length; i++) {
        output[i] += scratch[i]*taps[1];
    }
    shift_mirror_posi(scratch, norm_denom, 1, input_length);
    for(int i=0; i<input_length; i++) {
        output[i] += scratch[i]*taps[3];
    }
    shift_mirror_posi(scratch, norm_denom, 2, input_length);
    for(int i=0; i<input_length; i++) {
        output[i] += scratch[i]*taps[4];
    }
    
    double sum = 0.0;
    for(int i=0; i<5; i++) {
        sum += taps[i];
    }
    for(int i=0; i<input_length; i++) {
        output[i] = output[i]/sum;
    }
}

void aec_calc_normalisation_spectrum_fp(double *inv_X_energy, double *X_energy, double *sigma_XX, double gamma_log2, double delta, int is_shadow) {
    double norm_denom[NUM_BINS], scratch[NUM_BINS];
    double gamma = pow(2.0, gamma_log2);
    double taps[5] = {0.5, 1, 1, 1, 0.5};
    if(!is_shadow) {
        for(int i=0; i<NUM_BINS; i++) {
            norm_denom[i] = sigma_XX[i]*gamma + (X_energy[i]);
        }
        vect_smooth(inv_X_energy, scratch, norm_denom, taps, NUM_BINS);
        for(int i=0; i<NUM_BINS; i++) {
            inv_X_energy[i] = inv_X_energy[i] + delta;
        }
    }
    else {
        for(int i=0; i<NUM_BINS; i++) {
            inv_X_energy[i] = X_energy[i] + delta;
        }
    }
    for(int i=0; i<NUM_BINS; i++) {
        inv_X_energy[i] = 1.0 / inv_X_energy[i];
    }
}

void test_aec_inv_X_energy_div_by_zero() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 1;
    unsigned main_filter_phases = 6;
    unsigned shadow_filter_phases = 2;
    
    aec_state_t main_state, shadow_state;
    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_shared_state_t aec_shared_state;

    aec_init(&main_state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);
    unsigned seed = 3507;

    aec_state_t *state_ptr = &shadow_state;
    for(int ch=0; ch<num_x_channels; ch++) {
        state_ptr->X_energy[ch].exp = pseudo_rand_int(&seed, -31, 32);
        state_ptr->X_energy[ch].hr = 0;
        for(int i=0; i<NUM_BINS; i++) {
            unsigned is_zero = pseudo_rand_uint32(&seed) % 2;
            if(is_zero) {
                state_ptr->X_energy[ch].data[i] = 0;
            }
            else {
                state_ptr->X_energy[ch].data[i] = INT_MAX;
            }
        }
    }
    state_ptr->delta.mant = 0;
    state_ptr->delta.exp = -1024;
    unsigned is_shadow = 1; // to just test inv_X_energy = 1/(X_energy + delta) 
    for(int ch=0; ch<num_x_channels; ch++) {
        aec_calc_normalisation_spectrum(state_ptr, ch, is_shadow);
    }
}
void test_aec_calc_normalisation_spectrum() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 2;
    unsigned main_filter_phases = 6;
    unsigned shadow_filter_phases = 2;

    aec_state_t main_state, shadow_state;
    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_shared_state_t aec_shared_state;

    aec_init(&main_state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);
    //declare floating point memory
    double X_energy_fp[AEC_MAX_X_CHANNELS][NUM_BINS], sigma_XX_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
    double inv_X_energy_fp[AEC_MAX_X_CHANNELS][NUM_BINS];

    unsigned seed = 46894;

    aec_state_t *state_ptr;
    unsigned max_diff = 0;
    for(int iter=0; iter<(1<<12)/F; iter++) {
        unsigned is_shadow = pseudo_rand_uint32(&seed) % 2;
        if(is_shadow) {
            state_ptr = &shadow_state;
        }
        else {
            state_ptr = &main_state;
        }
        for(int ch=0; ch<num_x_channels; ch++) {
            state_ptr->X_energy[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state_ptr->X_energy[ch].hr = pseudo_rand_uint32(&seed) % 4;

            state_ptr->shared_state->sigma_XX[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state_ptr->shared_state->sigma_XX[ch].hr = pseudo_rand_uint32(&seed) % 4;
            for(int i=0; i<NUM_BINS; i++) {
                state_ptr->X_energy[ch].data[i] = (pseudo_rand_int32(&seed) & 0x7fffffff) >> state_ptr->X_energy[ch].hr; //energy is positive
                X_energy_fp[ch][i] = ldexp(state_ptr->X_energy[ch].data[i], state_ptr->X_energy[ch].exp);

                state_ptr->shared_state->sigma_XX[ch].data[i] = (pseudo_rand_int32(&seed) & 0x7fffffff) >> state_ptr->shared_state->sigma_XX[ch].hr; //sigma_XX is positive
                sigma_XX_fp[ch][i] = ldexp(state_ptr->shared_state->sigma_XX[ch].data[i], state_ptr->shared_state->sigma_XX[ch].exp);                    
            }
            state_ptr->delta.exp = -32 - (pseudo_rand_uint32(&seed) & 63);
            state_ptr->delta.mant = pseudo_rand_int32(&seed) & 0x7fffffff;
            if(state_ptr->delta.mant == 0) {
                state_ptr->delta = state_ptr->shared_state->config_params.aec_core_conf.delta_min;
            }

            double delta_fp = ldexp(state_ptr->delta.mant, state_ptr->delta.exp);
            for(int ch=0; ch<num_x_channels; ch++) {
                aec_calc_normalisation_spectrum_fp(inv_X_energy_fp[ch], X_energy_fp[ch], sigma_XX_fp[ch], 5, delta_fp, is_shadow);
            }
            for(int ch=0; ch<num_x_channels; ch++) {
                aec_calc_normalisation_spectrum(state_ptr, ch, is_shadow);
            }
        }
        for(int ch=0; ch<num_x_channels; ch++) {
            unsigned diff = vector_int32_maxdiff((int32_t*)&state_ptr->inv_X_energy[ch].data[0], state_ptr->inv_X_energy[ch].exp, (double*)inv_X_energy_fp[ch], 0, NUM_BINS);
            TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<16, diff, "inv_X_energy diff too large.");

            if(diff > max_diff) max_diff = diff;
            //printf("iter %d diff %d\n",iter, diff);
        }
    }
    printf("max_diff %d\n", max_diff);
}
