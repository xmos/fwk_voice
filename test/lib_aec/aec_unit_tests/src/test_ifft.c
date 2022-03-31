// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

double sine_lut_ifft[AEC_PROC_FRAME_LENGTH / 4 + 1];
//In-place N-pt complex IFFT
void aec_inverse_fft_fp(complex_double_t *input, int length) {
    bit_reverse     ((complex_double_t *)input, length);
    inverse_fft     ((complex_double_t *)input, length, sine_lut_ifft);
}

void test_ifft() {
    unsigned num_y_channels = 2;
    unsigned num_x_channels = 2;
    unsigned main_filter_phases = 6;
    unsigned shadow_filter_phases = 2;

    aec_state_t main_state, shadow_state;
    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_shared_state_t aec_shared_state;

    aec_init(&main_state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);

    unsigned seed = 78431;
    int32_t DWORD_ALIGNED new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
    complex_double_t DWORD_ALIGNED ref[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];

    //Init FFT for reference
    make_sine_table(sine_lut_ifft, AEC_PROC_FRAME_LENGTH);
    unsigned max_diff = 0;
    for(unsigned itt=0;itt<(1<<10)/F;itt++) {
        aec_frame_init(&main_state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);
        int call_type = pseudo_rand_uint32(&seed) % 2; //Error->error or Y_hat->y_hat IFFT
        int is_shadow = pseudo_rand_uint32(&seed) % 2;
        aec_state_t *state_ptr = (is_shadow == 1) ? &shadow_state : &main_state;
        bfp_complex_s32_t *ifft_in;
        bfp_s32_t *ifft_out;
        if(call_type == 0) { //Error->error
            ifft_in = &state_ptr->Error[0];
            ifft_out = &state_ptr->error[0];
        }
        else { //Y_hat->y_hat
            ifft_in = &state_ptr->Y_hat[0];
            ifft_out = &state_ptr->y_hat[0];
        }
        for(int ch=0; ch<num_y_channels; ch++) {
            ifft_in[ch].exp = - (int) (pseudo_rand_uint32(&seed)%50); //Between 0 and -49
            ifft_in[ch].hr = pseudo_rand_uint32(&seed)%16; //Up to 15 bits HR

            /* Generate N/2 complex frequency domain samples for DUT.
             * Generate N complex frequecny domain samples for Ref. The REF IFFT
             * is a complex N-pt IFFT. However the N complex ref freq domain samples are
             * symmetric around nyquist in such a way that the result of the REF complex
             * N-pt IFFT is real */
            for(int i=0; i<AEC_PROC_FRAME_LENGTH/2; i++) {
                ifft_in[ch].data[i].re = pseudo_rand_int32(&seed) >> ifft_in[ch].hr;
                ifft_in[ch].data[i].im = pseudo_rand_int32(&seed) >> ifft_in[ch].hr;

                ref[ch][i].re = ldexp(ifft_in[ch].data[i].re, ifft_in[ch].exp);
                ref[ch][i].im = ldexp(ifft_in[ch].data[i].im, ifft_in[ch].exp);
                //Generate (N/2+1) to (N-1) indexed bins based on symmetry around Nyquist
                if(i){
                    ref[ch][AEC_PROC_FRAME_LENGTH - i].re =  ref[ch][i].re;
                    ref[ch][AEC_PROC_FRAME_LENGTH - i].im = -ref[ch][i].im;
                }
            }
            //Unpack Nyquist into bin N/2
            ref[ch][AEC_PROC_FRAME_LENGTH/2].re = ref[ch][0].im;
            ref[ch][AEC_PROC_FRAME_LENGTH/2].im = 0;
            ref[ch][0].im = 0;
            ifft_in[ch].data[AEC_PROC_FRAME_LENGTH/2].re = ifft_in[ch].data[0].im; 
            ifft_in[ch].data[AEC_PROC_FRAME_LENGTH/2].im = 0;
            ifft_in[ch].data[0].im = 0;
        }
        
        //DUT IFFT
        for(int ch=0; ch<num_y_channels; ch++) {
            //printf("addr in: 0x%08x, addr out: 0x%08x\n",&ifft_in[ch].data[0], &ifft_out[ch].data[0]);
            aec_inverse_fft(&ifft_out[ch], &ifft_in[ch]);
        }
        /* N-pt complex freq domain data ->N-pt complex IFFT-> N-pt complex time domain data.
         * However the input freq domain is made to be symmetric around Nyquist in a way such that
         * the N-pt complex IFFT time domain output is real. Since IFFT is done in-place, the imaginary
         * fields of all the N complex time domain samples will be 0
         * */

        for(int ch=0; ch<num_y_channels; ch++) {
            //printf("addr in: 0x%08x, addr out: 0x%08x\n",&ifft_in[ch].data[0], &ifft_out[ch].data[0]);
            aec_inverse_fft_fp(&ref[ch][0], AEC_PROC_FRAME_LENGTH);
        }

        //Compare results
        for(int ch=0; ch<num_y_channels; ch++) {
            double ref_re[AEC_PROC_FRAME_LENGTH];
            for(int i=0; i<AEC_PROC_FRAME_LENGTH; i++) {
                ref_re[i] = ref[ch][i].re;
            }
            unsigned diff = vector_int32_maxdiff((int32_t*)&ifft_out[ch].data[0], ifft_out[ch].exp, (double*)&ref_re[0], 0, AEC_PROC_FRAME_LENGTH);
            max_diff = (diff > max_diff) ? diff : max_diff;
            TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<5, diff, "IFFT diff too large.");
            TEST_ASSERT_EQUAL_UINT32_MESSAGE(AEC_PROC_FRAME_LENGTH, ifft_out[ch].length, "IFFT output length incorrect");
        }
    }
    printf("max_diff %d\n", max_diff);
}
