// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

static void update_mapping(int *mapping, int num_phases)
{
    //mapping gives indexes into X_fifo from most recent phases to least recent phase
    int last_phase = mapping[num_phases - 1];
    //move phases one down
    for(int i=num_phases-1; i>=1; i--) {
        mapping[i] = mapping[i-1];
    }
    mapping[0] = last_phase;
}

void aec_calc_X_fifo_energy_fp(
        double (*X_energy)[NUM_BINS],
        double *max_X_energy,
        complex_double_t (*X)[NUM_BINS],
        complex_double_t (*X_fifo)[AEC_MAIN_FILTER_PHASES][NUM_BINS],
        const int *mapping,
        unsigned num_channels,
        unsigned num_phases,
        int recalc_bin)
{
    int last_ph = mapping[num_phases - 1];
    for(unsigned ch=0; ch<num_channels; ch++) {
        for(int i=0; i<NUM_BINS; i++) {
            double temp = (X_fifo[ch][last_ph][i].re * X_fifo[ch][last_ph][i].re) + (X_fifo[ch][last_ph][i].im * X_fifo[ch][last_ph][i].im);
            
            X_energy[ch][i] -= temp; //subtract energy of phase rolling out
            temp = (X[ch][i].re * X[ch][i].re) + (X[ch][i].im * X[ch][i].im);
            X_energy[ch][i] += temp; //Add latest X data energy
        }
        //do full calculation for recalc_bin
        //pick up newest num_phases -1 and the new X data
        double sum = (X[ch][recalc_bin].re * X[ch][recalc_bin].re) + (X[ch][recalc_bin].im * X[ch][recalc_bin].im);
        for(unsigned p=0; p<num_phases-1; p++) {
            int ph = mapping[p];
            sum += (X_fifo[ch][ph][recalc_bin].re * X_fifo[ch][ph][recalc_bin].re) + (X_fifo[ch][ph][recalc_bin].im * X_fifo[ch][ph][recalc_bin].im);
        }
        X_energy[ch][recalc_bin] = sum;
        max_X_energy[ch] = X_energy[ch][0];
        for(int i=1; i<NUM_BINS; i++) {
            if(X_energy[ch][i] > max_X_energy[ch]) {max_X_energy[ch] = X_energy[ch][i];}
        }
    }
}

void update_X_fifo_fp(
        complex_double_t (*X_fifo)[AEC_MAIN_FILTER_PHASES][NUM_BINS],
        int *mapping,
        complex_double_t (*X)[NUM_BINS],
        unsigned num_channels,
        unsigned num_phases
        )
{
    update_mapping(mapping, num_phases);
    //mapping[0] points to the phase that has rolled out of num_phases window and is now ready to be updated with new data thus becoming the most recent phase
    //printf("newest phase %d\n",mapping[0]);
    for(int ch=0; ch<num_channels; ch++) {
        for(int i=0; i<NUM_BINS; i++) {
            X_fifo[ch][mapping[0]][i].re = X[ch][i].re;
            X_fifo[ch][mapping[0]][i].im = X[ch][i].im;
        }
    }
    


}

void test_update_total_X_energy() {
    unsigned num_y_channels = 1;
    unsigned num_x_channels = 1;
    unsigned main_filter_phases = AEC_MAIN_FILTER_PHASES - 1;
    unsigned shadow_filter_phases = AEC_MAIN_FILTER_PHASES - 5;

    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_state_t state, shadow_state;
    aec_shared_state_t aec_shared_state;
    aec_init(&state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);
    
    unsigned X_energy_recalc_bin = 0;
    complex_s32_t X[AEC_MAX_X_CHANNELS][NUM_BINS];
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        bfp_complex_s32_init(&state.shared_state->X[ch], X[ch], 0, NUM_BINS, 0);
    }

    //Initialise floating point stuff. mapping, X_energy_fp and X_fifo_fp
    int mapping[AEC_MAIN_FILTER_PHASES];
    complex_double_t X_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
    double X_energy_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
    double X_energy_shadow_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
    double max_X_energy_fp[AEC_MAX_X_CHANNELS], max_X_energy_shadow_fp[AEC_MAX_X_CHANNELS];
    complex_double_t X_fifo_fp[AEC_MAX_X_CHANNELS][AEC_MAIN_FILTER_PHASES][NUM_BINS];
    for(unsigned ch=0; ch<num_x_channels; ch++) {
        for(unsigned bin=0; bin<NUM_BINS; bin++) {
            X_energy_fp[ch][bin] = 0.0;
            X_energy_shadow_fp[ch][bin] = 0.0;
        }
        for(int p=0; p<state.num_phases; p++) {
            for(int bin=0; bin<NUM_BINS; bin++) {
                X_fifo_fp[ch][p][bin].re = 0.0;
                X_fifo_fp[ch][p][bin].im = 0.0;
            }
            mapping[p] = p;
        }
    } 
    
    double max_diff_percentage_shadow = 0.0;
    double max_diff_percentage = 0.0;
    int max_diff = 0;
    unsigned seed = 2;
    for(unsigned iter=0; iter<(1<<12)/F; iter++) {
        for(unsigned ch=0; ch<num_x_channels; ch++) {
            bfp_complex_s32_t *X_ptr = &state.shared_state->X[ch];
            X_ptr->exp = pseudo_rand_int(&seed, -3, 4) - 30;
            X_ptr->hr = (pseudo_rand_uint32(&seed) % 3);
            
            //Generate X
            for(unsigned bin=0; bin<NUM_BINS; bin++)
            {
                X_ptr->data[bin].re = pseudo_rand_int32(&seed) >> X_ptr->hr;
                X_ptr->data[bin].im = pseudo_rand_int32(&seed) >> X_ptr->hr;
                X_fp[ch][bin].re = ldexp(X_ptr->data[bin].re, X_ptr->exp);
                X_fp[ch][bin].im = ldexp(X_ptr->data[bin].im, X_ptr->exp);
            }
        }
        
        //Calculate X_energy
        for(unsigned ch=0; ch<num_x_channels; ch++) {
            aec_calc_X_fifo_energy(&state, ch, X_energy_recalc_bin);
            aec_calc_X_fifo_energy(&shadow_state, ch, X_energy_recalc_bin);
        }
        aec_calc_X_fifo_energy_fp(X_energy_fp, &max_X_energy_fp[0], X_fp, X_fifo_fp, mapping, num_x_channels, state.num_phases, X_energy_recalc_bin); 
        aec_calc_X_fifo_energy_fp(X_energy_shadow_fp, &max_X_energy_shadow_fp[0], X_fp, X_fifo_fp, mapping, num_x_channels, shadow_state.num_phases, X_energy_recalc_bin); 
        //Update X_fifo
        for(unsigned ch=0; ch<num_x_channels; ch++) {
            aec_update_X_fifo_and_calc_sigmaXX(&state, ch);
        }
        update_X_fifo_fp(X_fifo_fp, mapping, X_fp, num_x_channels, state.num_phases);

        aec_update_X_fifo_1d(&state);
        aec_update_X_fifo_1d(&shadow_state);

        //Check 1d fifo update
        int count = 0;
        for(int i=0; i<num_x_channels; i++) {
            for(int j=0; j<state.num_phases; j++) {
                TEST_ASSERT_EQUAL_INT32(state.X_fifo_1d[count].data, state.shared_state->X_fifo[i][j].data);
                TEST_ASSERT_EQUAL_INT32(state.X_fifo_1d[count].exp, state.shared_state->X_fifo[i][j].exp);
                TEST_ASSERT_EQUAL_INT32(state.X_fifo_1d[count].hr, state.shared_state->X_fifo[i][j].hr);
                TEST_ASSERT_EQUAL_INT32(state.X_fifo_1d[count].length, state.shared_state->X_fifo[i][j].length);
                count++;
            }
        }
        //printf("iter %d. done memcmp\n", iter);

        //compare X_energy
        //printf("iter %d\n",iter);
        for(unsigned ch=0; ch < num_x_channels; ch++){
            bfp_s32_t *X_energy_ptr = &state.X_energy[ch];
            bfp_s32_t *X_energy_shadow_ptr = &shadow_state.X_energy[ch];
            for(unsigned i=0; i<NUM_BINS; i++) {
                double ref_double = X_energy_fp[ch][i];
                double dut_double = ldexp(X_energy_ptr->data[i], X_energy_ptr->exp);
                double diff_double = ref_double - dut_double;
                if(diff_double < 0.0) diff_double = -diff_double;
                double diff_percentage = (diff_double/ref_double) * 100;
                if(diff_percentage > max_diff_percentage) max_diff_percentage = diff_percentage;
                if(diff_double > 0.0002*(ref_double < 0.0 ? -ref_double : ref_double) + pow(10, -8))
                {
                    printf("Main filter: iter %d. ch: %d, bin: %d, diff %f outside pass limits. ref %f, dut %f\n", iter, ch, i, diff_double, ref_double, dut_double);
                    printf("Main filter: ch %d, bin %d: ref (%f), dut (0x%lx, %d)\n",ch, i, ref_double, X_energy_ptr->data[i], X_energy_ptr->exp);                    
                    assert(0);
                }
                ref_double = X_energy_shadow_fp[ch][i];
                dut_double = ldexp(X_energy_shadow_ptr->data[i], X_energy_shadow_ptr->exp);
                diff_double = ref_double - dut_double;
                if(diff_double < 0.0) diff_double = -diff_double;
                diff_percentage = (diff_double/ref_double) * 100;
                if(diff_percentage > max_diff_percentage_shadow) max_diff_percentage_shadow = diff_percentage;
                if(diff_double > 0.002*(ref_double < 0.0 ? -ref_double : ref_double) + pow(10, -8))
                {
                    printf("Shadow filter: iter %d, ch: %d, bin: %d, diff %f outside pass limits. ref %f, dut %f\n", iter, ch, i, diff_double, ref_double, dut_double);
                    printf("Shadow filter: ch %d, bin %d: ref (%f), dut (0x%lx, %d)\n",ch, i, ref_double, X_energy_shadow_ptr->data[i], X_energy_shadow_ptr->exp);                    
                    assert(0);
                }
            }
            //max_X_energy
            double ref_double = max_X_energy_fp[ch];
            double dut_double = ldexp(state.max_X_energy[ch].mant, state.max_X_energy[ch].exp);
            double diff_double = ref_double - dut_double;
            if(diff_double < 0.0) {diff_double = -diff_double;}
            if(diff_double > 0.0002*(ref_double < 0.0 ? -ref_double : ref_double) + pow(10, -8))
            {
                printf("Main filter: max_X_energy, iter %d. ch: %d, diff %f outside pass limits. ref %f, dut %f\n", iter, ch, diff_double, ref_double, dut_double);
                assert(0);
            }

            ref_double = max_X_energy_shadow_fp[ch];
            dut_double = ldexp(shadow_state.max_X_energy[ch].mant, shadow_state.max_X_energy[ch].exp);
            diff_double = ref_double - dut_double;
            if(diff_double < 0.0) {diff_double = -diff_double;}
            if(diff_double > 0.002*(ref_double < 0.0 ? -ref_double : ref_double) + pow(10, -8))
            {
                printf("Shadow filter: max_X_energy, iter %d. ch: %d, diff %f outside pass limits. ref %f, dut %f\n", iter, ch, diff_double, ref_double, dut_double);
                assert(0);
            }
        }
        X_energy_recalc_bin += 1;
        if(X_energy_recalc_bin == (AEC_PROC_FRAME_LENGTH/2) + 1) {
            X_energy_recalc_bin = 0;
        }        
    }
    printf("max_diff_percentage = %f\n",max_diff_percentage);
    printf("max_diff_percentage_shadow = %f\n",max_diff_percentage_shadow);
}
