// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

void aec_update_sigma_XX_fp (double (*sigma_XX)[NUM_BINS], double *sum_X_energy, complex_double_t (*X_fp)[NUM_BINS], unsigned num_channels, int sigma_xx_shift)
{
    for(unsigned x_ch=0; x_ch<num_channels; x_ch++) {
        sum_X_energy[x_ch] = 0.0;
        for(unsigned i=0; i<NUM_BINS; i++) {
            double energy = (X_fp[x_ch][i].re*X_fp[x_ch][i].re)+(X_fp[x_ch][i].im*X_fp[x_ch][i].im);
            sum_X_energy[x_ch] += energy;
            sigma_XX[x_ch][i] = sigma_XX[x_ch][i]*(1.0 - ldexp(1.0, -sigma_xx_shift));
            sigma_XX[x_ch][i] += (energy*ldexp(1.0, -sigma_xx_shift));            
        }
    }
}

static void update_mapping(int *mapping, int num_phases)
{
    int last_phase = mapping[num_phases - 1];
    //move phases one down
    for(int i=num_phases-1; i>=1; i--) {
        mapping[i] = mapping[i-1];
    }
    mapping[0] = last_phase;
}

void test_update_sigma_XX() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 1;
    unsigned num_phases = 10;
    aec_memory_pool_t aec_memory_pool;
    aec_state_t state;
    aec_shared_state_t aec_shared_state;
    aec_init(&state, NULL, &aec_shared_state, (uint8_t*)&aec_memory_pool, NULL, num_y_channels, num_x_channels, num_phases, 0);
    complex_s32_t X[AEC_MAX_X_CHANNELS][NUM_BINS];
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&state.shared_state->X[ch], X[ch], 0, NUM_BINS, 0);
    }
    complex_double_t X_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
    double sigma_XX_fp[AEC_MAX_X_CHANNELS][NUM_BINS], sum_X_energy_fp[AEC_MAX_X_CHANNELS];
    //initialise floating point stuff. sigma_xx_fp
    for(unsigned x_ch=0; x_ch<num_x_channels; x_ch++) {
        for(unsigned bin=0; bin<NUM_BINS; bin++) {
            sigma_XX_fp[x_ch][bin] = 0.0;
        }
    }
    
    int mapping[AEC_MAIN_FILTER_PHASES];
    bfp_complex_s32_t X_fifo_check[AEC_MAX_X_CHANNELS][AEC_MAIN_FILTER_PHASES];
    for(int ch=0; ch<num_x_channels; ch++) {
        for(int i=0; i<num_phases; i++) {
            mapping[i] = i;
            X_fifo_check[ch][i] = state.shared_state->X_fifo[ch][i];
        }
    }
    int max_diff = 0;
    int max_diff_sum_X_energy = 0;
    unsigned seed = 2;
    for(unsigned iter=0; iter<(1<<12)/F; iter++) {
        for(unsigned ch=0; ch<num_x_channels; ch++) {
            bfp_complex_s32_t *X_ptr = &state.shared_state->X[ch];
            X_ptr->exp = pseudo_rand_int(&seed, -31, 32);
            X_ptr->hr = (pseudo_rand_uint32(&seed) % 3);

            for(unsigned bin=0; bin<NUM_BINS; bin++)
            {
                X_ptr->data[bin].re = pseudo_rand_int32(&seed) >> X_ptr->hr;
                X_ptr->data[bin].im = pseudo_rand_int32(&seed) >> X_ptr->hr;
                X_fp[ch][bin].re = ldexp(X_ptr->data[bin].re, X_ptr->exp);
                X_fp[ch][bin].im = ldexp(X_ptr->data[bin].im, X_ptr->exp);
            }
        }

        for(unsigned ch=0; ch<num_x_channels; ch++) {
            aec_update_X_fifo_and_calc_sigmaXX(&state, ch);
        }
        aec_update_sigma_XX_fp(sigma_XX_fp, sum_X_energy_fp, X_fp, num_x_channels, state.shared_state->config_params.aec_core_conf.sigma_xx_shift); 
        
        for(unsigned ch=0; ch < num_x_channels; ch++){
            //printf("%f, %f\n",sum_X_energy_fp[ch], ldexp(state.shared_state->sum_X_energy[ch].mant, state.shared_state->sum_X_energy[ch].exp));
            unsigned diff = vector_int32_maxdiff((int32_t*)&state.shared_state->sum_X_energy[ch].mant, state.shared_state->sum_X_energy[ch].exp, (double*)&sum_X_energy_fp[ch], 0, 1);
            max_diff_sum_X_energy = (diff > max_diff_sum_X_energy) ? diff : max_diff_sum_X_energy;
            TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<3, diff, "sum_X_energy diff too large");
        }
        //compare sigma_XX
        for(unsigned ch=0; ch < num_x_channels; ch++){
            bfp_s32_t *sigma_ptr = &state.shared_state->sigma_XX[ch];
            for(unsigned i=0; i<NUM_BINS; i++) {
                uint32_t expected = double_to_int32(sigma_XX_fp[ch][i], sigma_ptr->exp);
                int diff = expected - sigma_ptr->data[i];
                if(diff < 0) diff = -diff;
                if(diff > max_diff) max_diff = diff;
                double dut_float = ldexp(sigma_ptr->data[i], sigma_ptr->exp);
                if(diff > (1 << 10)) {
                    printf("Fail. Iter %d, ch %d, bin %d, ref %f, dut (%ld, %d), %f\n",iter, ch, i, sigma_XX_fp[ch][i], sigma_ptr->data[i], sigma_ptr->exp, dut_float);
                }
                TEST_ASSERT_INT32_WITHIN_MESSAGE(1<<10, expected, sigma_ptr->data[i], "sigma_xx broke");
            }
        }
        //check X_fifo update
        update_mapping(mapping, num_phases);
        for(unsigned ch=0; ch < num_x_channels; ch++){
            bfp_complex_s32_t *X_ptr = &state.shared_state->X[ch];
            TEST_ASSERT_EQUAL_INT32(state.shared_state->X_fifo[ch][0].exp, X_ptr->exp);
            TEST_ASSERT_EQUAL_INT32(state.shared_state->X_fifo[ch][0].hr, X_ptr->hr);
            TEST_ASSERT_EQUAL_INT32(state.shared_state->X_fifo[ch][0].length, X_ptr->length);
            if(memcmp(state.shared_state->X_fifo[ch][0].data, X_ptr->data, X_ptr->length*sizeof(X_ptr->data[0])))
            {
                printf("X data mismatch\n");
                assert(0);
            }
            for(unsigned ph=0; ph<num_phases; ph++) {
                TEST_ASSERT_EQUAL_INT32_MESSAGE(state.shared_state->X_fifo[ch][ph].data, X_fifo_check[ch][mapping[ph]].data, "X_fifo data ptr mismatch");
            }
        }
    }
    printf("max_diff = %d\n",max_diff);
    printf("max_diff_sum_X_energy = %d\n",max_diff_sum_X_energy);
}
