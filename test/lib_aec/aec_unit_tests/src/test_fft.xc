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

double sine_lut[AEC_PROC_FRAME_LENGTH / 4 + 1];
//In-place N point complex FFT
void aec_forward_fft_fp(complex_double_t *input, int length) {
    att_bit_reverse((dsp_complex_fp*)input, length);
    att_forward_fft((dsp_complex_fp*)input, length, sine_lut);
}

void test_fft() {
    unsafe {
        unsigned num_y_channels = 1;
        unsigned num_x_channels = 2;
        unsigned main_filter_phases = 6;
        unsigned shadow_filter_phases = 2;
        
        aec_state_t main_state, shadow_state;
        aec_memory_pool_t aec_memory_pool;
        aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
        aec_shared_state_t aec_shared_state;

        aec_init(&main_state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);

        int32_t [[aligned(8)]] new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
        complex_double_t [[aligned(8)]] ref[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];
        aec_state_t *state_ptr;
        unsigned seed = 83472;
        //Init FFT for reference
        att_make_sine_table(sine_lut, AEC_PROC_FRAME_LENGTH);

        //3 FFTs in AEC: Y, X, Error
        int max_diff = 0;
        for(int iter=0; iter<(1<<10)/F; iter++) {
            unsigned call_type = att_random_uint32(seed) % 3;
            unsigned is_shadow = att_random_uint32(seed) % 2;
            //printf("call_type %d, is_shadow %d\n",call_type, is_shadow);
            if(is_shadow) {
                state_ptr = &main_state;
            }
            else {
                state_ptr = &shadow_state;
            }
            aec_frame_init(&main_state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);        
            bfp_s32_t *fft_in;
            bfp_complex_s32_t *fft_out;
            int num_channels;

            for(int ch=0; ch<num_y_channels; ch++) {
                //state_ptr->error is initialised in the Error->error ifft aec_ifft() call with state_ptr->error as input. Initialising here for
                //standalone testing.
                bfp_s32_init(&state_ptr->error[ch], (int32_t*)&state_ptr->Error[ch].data[0], 0, AEC_PROC_FRAME_LENGTH, 0);
            }
            if(call_type == 0) { //FFT Y
                fft_in = &state_ptr->shared_state->y[0];
                fft_out = &state_ptr->shared_state->Y[0];
                num_channels = num_y_channels;
            }
            else if(call_type == 1) { //FFT X
                fft_in = &state_ptr->shared_state->x[0];
                fft_out = &state_ptr->shared_state->Y[0];
                num_channels = num_x_channels;
            }
            else { //FFT Error
                fft_in = &state_ptr->error[0];
                fft_out = &state_ptr->Error[0];
                num_channels = num_y_channels;
            }

            //generate input
            /* Generate inputs for the N-pt real FFT. Reference does an in-place N-pt complex FFT to generate
             * N complex frequency domain values so store reference input in a N point complex input array with
             * imaginary fields set to 0.
             * */
            for(int ch=0; ch<num_channels; ch++) {
                fft_in[ch].exp = - (int) (att_random_uint32(seed)%50); //Between 0 and -49
                fft_in[ch].hr = att_random_uint32(seed)%16; //Up to 15 bits HR

                for(int i=0; i<AEC_PROC_FRAME_LENGTH; i++){
                    fft_in[ch].data[i] = sext(att_random_int32(seed), 32-fft_in[ch].hr);
                    ref[ch][i].re = att_int32_to_double(fft_in[ch].data[i], fft_in[ch].exp);
                    ref[ch][i].im = 0;
                }
            }
            //DUT FFT
            for(int ch=0; ch<num_channels; ch++) {
                aec_forward_fft(&fft_out[ch], &fft_in[ch]);
            }
            //Ref FFT
            for(int ch=0; ch<num_channels; ch++) {
                aec_forward_fft_fp(&ref[ch][0], AEC_PROC_FRAME_LENGTH);
            }
            //Compare first (N/2+1) complex values
            for(int ch=0; ch<num_channels; ch++) {
                unsigned diff = att_bfp_vector_int32((int32_t*)&fft_out[ch].data[0], fft_out[ch].exp, (double*)&ref[ch][0], 0, (AEC_PROC_FRAME_LENGTH/2+1)*2);
                max_diff = (diff > max_diff) ? diff : max_diff;
                TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<5, diff, "FFT diff too large.");
                TEST_ASSERT_EQUAL_UINT32_MESSAGE((AEC_PROC_FRAME_LENGTH/2)+1, fft_out[ch].length, "FFT output length incorrect");
            }
        }
        printf("max_diff = %d\n",max_diff);
    }
}
