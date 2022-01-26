// Copyright 2018-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
extern "C"{
    #include "aec_defines.h"
    #include "aec_api.h"
}

#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)
 
void aec_calc_T_fp(
        dsp_complex_fp (*T)[AEC_MAX_X_CHANNELS][NUM_BINS],
        dsp_complex_fp (*Error)[NUM_BINS],
        double (*inv_X_energy)[NUM_BINS],
        double (*mu)[AEC_MAX_X_CHANNELS],
        int y_channels,
        int x_channels) {
    //T[x_ch] = self.mu[y_ch, x_ch] * Inv_x_energy[x_ch] * Error[y_ch] / self.K
    for(int ych=0; ych<y_channels; ych++) {
        for(int xch=0; xch<x_channels; xch++) {
            for(int i=0; i<NUM_BINS; i++) {
                T[ych][xch][i].re = Error[ych][i].re * inv_X_energy[xch][i] * mu[ych][xch];
                T[ych][xch][i].im = Error[ych][i].im * inv_X_energy[xch][i] * mu[ych][xch];
            }
        }
    }
}

void test_calc_T() {
    unsafe {
        unsigned num_y_channels = 2;
        unsigned num_x_channels = 2;
        unsigned main_filter_phases = AEC_MAIN_FILTER_PHASES - 1;
        unsigned shadow_filter_phases = AEC_MAIN_FILTER_PHASES - 1;

        aec_memory_pool_t aec_memory_pool;
        aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
        aec_state_t state, shadow_state;
        aec_shared_state_t aec_shared_state;
        aec_init(&state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);

        //initialise float arrays
        dsp_complex_fp Error_fp[AEC_MAX_Y_CHANNELS][NUM_BINS];
        double inv_X_energy_fp[AEC_MAX_X_CHANNELS][NUM_BINS];
        double mu_fp[AEC_MAX_Y_CHANNELS][AEC_MAX_X_CHANNELS];
        dsp_complex_fp T_fp[AEC_MAX_Y_CHANNELS][AEC_MAX_X_CHANNELS][NUM_BINS];
        
        unsigned seed = 45;
        double max_diff_percentage = 0.0;
        for(int iter=0; iter<(1<<11)/F; iter++) {
            int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
            aec_frame_init(&state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);
            aec_state_t *state_ptr;
            int is_main_filter = att_random_uint32(seed) % 2;
            if(is_main_filter) {
                state_ptr = &state;
            }
            else {
                state_ptr = &shadow_state;
            }

            for(int ch=0; ch<num_y_channels; ch++) {
                state_ptr->Error[ch].exp = sext(att_random_int32(seed), 3) - 31;
                state_ptr->Error[ch].hr = att_random_uint32(seed) % 3;                
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->Error[ch].data[i].re = att_random_int32(seed) >> state_ptr->Error[ch].hr;
                    state_ptr->Error[ch].data[i].im = att_random_int32(seed) >> state_ptr->Error[ch].hr;

                    Error_fp[ch][i].re = att_int32_to_double(state_ptr->Error[ch].data[i].re, state_ptr->Error[ch].exp);
                    Error_fp[ch][i].im = att_int32_to_double(state_ptr->Error[ch].data[i].im, state_ptr->Error[ch].exp);
                }
            }
            //initialise inv_X_energy
            for(int ch=0; ch<num_x_channels; ch++) {
                state_ptr->inv_X_energy[ch].exp = sext(att_random_int32(seed), 3) - 31;
                state_ptr->inv_X_energy[ch].hr = (att_random_uint32(seed) % 3);
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->inv_X_energy[ch].data[i] = att_random_int32(seed) >> state_ptr->inv_X_energy[ch].hr;
                    inv_X_energy_fp[ch][i] = att_int32_to_double(state_ptr->inv_X_energy[ch].data[i], state_ptr->inv_X_energy[ch].exp);
                }
            }
            //initialise mu
            for(int ych=0; ych<num_y_channels; ych++) {
                for(int xch=0; xch<num_y_channels; xch++) {
                    state_ptr->mu[ych][xch].exp = sext(att_random_int32(seed), 3) - 31;
                    int hr = att_random_uint32(seed) % 3;                
                    state_ptr->mu[ych][xch].mant = att_random_int32(seed) >> hr;

                    mu_fp[ych][xch] = att_int32_to_double(state_ptr->mu[ych][xch].mant, state_ptr->mu[ych][xch].exp);
                }
            }

            /*if(iter == 293) {
                printf("DUT: Error: exp %d, hr %d, re,im = (%d, %d). inv_X_energy: exp %d, hr %d, data %d\n", state_ptr->Error[1].exp, state_ptr->Error[1].hr, state_ptr->Error[1].data[79].re, state_ptr->Error[1].data[79].im, state_ptr->inv_X_energy[0].exp, state_ptr->inv_X_energy[0].hr, state_ptr->inv_X_energy[0].data[79]);
                printf("DUT: Error = (%.15f, %.15f), inv_X_energy = (%.15f)\n", att_int32_to_double(state_ptr->Error[1].data[79].re, state_ptr->Error[1].exp), att_int32_to_double(state_ptr->Error[1].data[79].im, state_ptr->Error[1].exp), att_int32_to_double(state_ptr->inv_X_energy[0].data[79], state_ptr->inv_X_energy[0].exp));
                printf("REF: Error = (%.15f, %.15f), inv_X_energy = (%.15f)\n", Error_fp[1][79].re, Error_fp[1][79].im, inv_X_energy_fp[0][79]);
                printf("mu = (exp: %d mant %d), %.15f\n", state_ptr->mu[1][0].exp, state_ptr->mu[1][0].mant, mu_fp[1][0]);
            }*/

            aec_calc_T_fp(T_fp, Error_fp, inv_X_energy_fp, mu_fp, num_y_channels, num_x_channels);

            for(int ych=0; ych<num_y_channels; ych++) {
                for(int xch=0; xch<num_x_channels; xch++) {
                    aec_calc_T(state_ptr, ych, xch);
                } 
                //Since T memory will be overwritten when computing for next y-channel, do error checking now
                for(int xch=0; xch<num_x_channels; xch++) {
                    for(int i=0; i<NUM_BINS; i++) {
                        double ref_fp = T_fp[ych][xch][i].re;
                        double dut_fp = att_int32_to_double(state_ptr->T[xch].data[i].re, state_ptr->T[xch].exp);
                        double diff = (ref_fp - dut_fp);
                        if(diff < 0.0) diff = -diff;
                        double diff_percentage = (diff / (ref_fp < 0.0 ? -ref_fp : ref_fp)) * 100;
                        /*if((iter == 293) && (ych == 1) && (xch == 0) && (i == 79)) {
                            printf("Re: bin %d, diff = %.15f, diff_percent = %f. DUT T (0x%x, %d, %.15f). Ref T %.15f\n", i, diff, diff_percentage, state_ptr->T[xch].data[i].re, state_ptr->T[xch].exp, att_int32_to_double(state_ptr->T[xch].data[i].re, state_ptr->T[xch].exp), T_fp[ych][xch][i].re);
                        }*/
                        if(diff_percentage > max_diff_percentage) max_diff_percentage = diff_percentage;
                        if(diff > 0.0002*(ref_fp < 0.0 ? -ref_fp : ref_fp) + pow(10, -8))
                        {
                            printf("Re fail: iter %d, ych %d, xch %d, bin %d\n",iter, ych, xch, i);
                            assert(0);
                        }

                        ref_fp = T_fp[ych][xch][i].im;
                        dut_fp = att_int32_to_double(state_ptr->T[xch].data[i].im, state_ptr->T[xch].exp);
                        diff = (ref_fp - dut_fp);
                        if(diff < 0.0) diff = -diff;
                        diff = abs(diff);
                        diff_percentage = (diff / (ref_fp < 0.0 ? -ref_fp : ref_fp)) * 100;
                        /*if((iter == 293) && (ych == 1) && (xch == 0) && (i == 79)) {
                            printf("Im: ych %d, xch %d, bin %d, diff = %f, diff_percent = %f\n", ych, xch, i, diff, diff_percentage);
                        }*/

                        if(diff_percentage > max_diff_percentage) max_diff_percentage = diff_percentage;
                        if(diff > 0.0002*(ref_fp < 0.0 ? -ref_fp : ref_fp) + pow(10, -8))
                        {
                            printf("Im fail: iter %d, ych %d, xch %d, bin %d\n",iter, ych, xch, i);
                            assert(0);
                        }
                    }
                }
            }
        }
    }
}
