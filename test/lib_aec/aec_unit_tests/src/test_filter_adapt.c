// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define TEST_NUM_Y (1)
#define TEST_NUM_X (2)
#define TEST_MAIN_PHASES (5)
#define TEST_SHADOW_PHASES (3)
#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

static double sine_lut_ifft[AEC_PROC_FRAME_LENGTH / 4 + 1];
static double sine_lut[AEC_PROC_FRAME_LENGTH / 4 + 1];
void aec_filter_adapt_fp(
        complex_double_t *H_hat,
        complex_double_t *X_fifo,
        complex_double_t *T,
        int bypass) {
    if(bypass) {
        return;
    }
    complex_double_t scratch[AEC_PROC_FRAME_LENGTH];
    int N = AEC_PROC_FRAME_LENGTH;
    
    for(int i=0; i<N/2+1; i++) {
        complex_double_t T_mult_conj_X;
        T_mult_conj_X.re = (T[i].re*X_fifo[i].re + T[i].im*X_fifo[i].im);
        T_mult_conj_X.im = (T[i].im*X_fifo[i].re - T[i].re*X_fifo[i].im);
        H_hat[i].re = H_hat[i].re + T_mult_conj_X.re;
        H_hat[i].im = H_hat[i].im + T_mult_conj_X.im;
    }
    //Generate 2nd half of the spectrum based on symmetry
    for(int i=0; i<N/2; i++) {
        scratch[i].re = H_hat[i].re;
        scratch[i].im = H_hat[i].im;

        if(i) {
            scratch[N-i].re = scratch[i].re;
            scratch[N-i].im = -scratch[i].im;
        }
        //Copy nyquist
        scratch[N/2].re = H_hat[N/2].re;
        scratch[N/2].im = H_hat[N/2].im;
    }
    //IFFT
    bit_reverse((complex_double_t *)scratch, N);
    inverse_fft((complex_double_t *)scratch, N, sine_lut_ifft);
    for(int i=AEC_FRAME_ADVANCE; i<AEC_PROC_FRAME_LENGTH; i++) {
        scratch[i].re = 0.0;
    }
    bit_reverse((complex_double_t*)scratch, N);
    forward_fft((complex_double_t*)scratch, N, sine_lut);
    
    for(int i=0; i<N/2+1; i++) {
        H_hat[i].re = scratch[i].re;
        H_hat[i].im = scratch[i].im;
    }
}
void test_aec_filter_adapt() {
    unsigned num_y_channels = TEST_NUM_Y;
    unsigned num_x_channels = TEST_NUM_X;
    unsigned main_filter_phases = TEST_MAIN_PHASES;
    unsigned shadow_filter_phases = TEST_SHADOW_PHASES;

    aec_state_t state, shadow_state;
    aec_memory_pool_t aec_memory_pool;
    aec_shadow_filt_memory_pool_t aec_shadow_memory_pool;
    aec_shared_state_t aec_shared_state;

    aec_init(&state, &shadow_state, &aec_shared_state, (uint8_t*)&aec_memory_pool, (uint8_t*)&aec_shadow_memory_pool, num_y_channels, num_x_channels, main_filter_phases, shadow_filter_phases);

    //Declare floating point arrays
    complex_double_t H_hat_fp[TEST_NUM_Y][TEST_NUM_X*TEST_MAIN_PHASES][NUM_BINS];
    complex_double_t X_fifo_fp[TEST_NUM_X][TEST_MAIN_PHASES][NUM_BINS];
    complex_double_t T_fp[TEST_NUM_X][NUM_BINS];

    //Init FFT for reference
    make_sine_table(sine_lut, AEC_PROC_FRAME_LENGTH);
    make_sine_table(sine_lut_ifft, AEC_PROC_FRAME_LENGTH);
    unsigned seed=578335;
    unsigned max_diff = 0.0;
    for(int itt=0; itt<(100)/F; itt++) {
        int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
        unsigned is_main = pseudo_rand_uint32(&seed) % 2;
        aec_state_t *state_ptr;
        if(is_main) {
            state_ptr = &state;
        }
        else {
            state_ptr = &shadow_state;
        }
        state_ptr->shared_state->config_params.aec_core_conf.bypass = pseudo_rand_uint32(&seed) % 2;
        unsigned test_l2_api = pseudo_rand_uint32(&seed) % 2;
        aec_frame_init(&state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);        
        //Generate H_hat
        for(int ch=0; ch<num_y_channels; ch++) {
            for(int ph=0; ph<num_x_channels*state_ptr->num_phases; ph++) {
                state_ptr->H_hat[ch][ph].exp = pseudo_rand_int(&seed, -31, 32);
                state_ptr->H_hat[ch][ph].hr = pseudo_rand_uint32(&seed) % 5;
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->H_hat[ch][ph].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->H_hat[ch][ph].hr;
                    state_ptr->H_hat[ch][ph].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->H_hat[ch][ph].hr;

                    H_hat_fp[ch][ph][i].re = ldexp(state_ptr->H_hat[ch][ph].data[i].re, state_ptr->H_hat[ch][ph].exp);
                    H_hat_fp[ch][ph][i].im = ldexp(state_ptr->H_hat[ch][ph].data[i].im, state_ptr->H_hat[ch][ph].exp);
                }
                //DC and Nyquist bin imaginary=0
                state_ptr->H_hat[ch][ph].data[0].im = 0;
                state_ptr->H_hat[ch][ph].data[NUM_BINS-1].im = 0;
                H_hat_fp[ch][ph][0].im = 0.0;
                H_hat_fp[ch][ph][NUM_BINS-1].im = 0.0;
            }
        }
        //Generate X_fifo, (always for number of phases in main_state)
        aec_state_t *main_state_ptr = &state;
        for(int ch=0; ch<num_x_channels; ch++) {
            for(int ph=0; ph<main_state_ptr->num_phases; ph++) {
                state_ptr->shared_state->X_fifo[ch][ph].exp = pseudo_rand_int(&seed, -31, 32);
                state_ptr->shared_state->X_fifo[ch][ph].hr = pseudo_rand_uint32(&seed) % 5;
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;

                    X_fifo_fp[ch][ph][i].re = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].re, state_ptr->shared_state->X_fifo[ch][ph].exp);
                    X_fifo_fp[ch][ph][i].im = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].im, state_ptr->shared_state->X_fifo[ch][ph].exp);
                }
                state_ptr->shared_state->X_fifo[ch][ph].data[0].im = 0;
                state_ptr->shared_state->X_fifo[ch][ph].data[NUM_BINS-1].im = 0;
                X_fifo_fp[ch][ph][0].im = 0.0;
                X_fifo_fp[ch][ph][NUM_BINS-1].im = 0.0;
            }
        }
        //Generate T
        for(int ch=0; ch<num_x_channels; ch++) {
            state_ptr->T[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state_ptr->T[ch].hr = pseudo_rand_uint32(&seed) % 5;
            for(int i=0; i<NUM_BINS; i++) {
                state_ptr->T[ch].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->T[ch].hr;
                state_ptr->T[ch].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->T[ch].hr;

                T_fp[ch][i].re = ldexp(state_ptr->T[ch].data[i].re, state_ptr->T[ch].exp);
                T_fp[ch][i].im = ldexp(state_ptr->T[ch].data[i].im, state_ptr->T[ch].exp);
            }
            state_ptr->T[ch].data[0].im = 0;
            state_ptr->T[ch].data[NUM_BINS-1].im = 0;
            T_fp[ch][0].im = 0.0;
            T_fp[ch][NUM_BINS-1].im = 0.0;
        }
        //aec init only initialises the 2d Xfifo. Since we're using the 1d fifo for error computation, call aec_update_X_fifo_1d()
        //to update the 1d Fifo
        aec_update_X_fifo_1d(state_ptr);

        //ref
        for(int ych=0; ych<num_y_channels; ych++) {
            for(int xch=0; xch<num_x_channels; xch++) {
                for(int p=0; p<state_ptr->num_phases; p++) {
                    aec_filter_adapt_fp(H_hat_fp[ych][xch*state_ptr->num_phases + p], X_fifo_fp[xch][p], T_fp[xch], state_ptr->shared_state->config_params.aec_core_conf.bypass);
                }
            }
        }
        //dut
        if(!test_l2_api) { 
            for(int ch=0; ch<num_y_channels; ch++) {
                aec_filter_adapt(state_ptr, ch);
            }
        }
        else {
            #define NUM_CHUNKS_PER_CH (4) //spread num_phases over 4 chunks for each y-channel
            if(!state_ptr->shared_state->config_params.aec_core_conf.bypass) {
                for(int c=0; c<num_y_channels; c++) {
                    int remaining_phases = num_x_channels * state_ptr->num_phases;
                    int start_phase=0;
                    int num_phases;
                    for(int t=0; t<NUM_CHUNKS_PER_CH; t++) {
                        int ch=c;
                        if((t == NUM_CHUNKS_PER_CH-1) || (remaining_phases <= 1))
                        {
                            num_phases = remaining_phases;
                            remaining_phases = 0;
                        }
                        else if(remaining_phases > 1) {
                            num_phases = (uint32_t)pseudo_rand_uint32(&seed) % remaining_phases;
                            remaining_phases -= num_phases;
                        }
                        for(int ph=start_phase; ph<start_phase+num_phases; ph++) {
                            aec_l2_adapt_plus_fft_gc(&state_ptr->H_hat[ch][ph], &state_ptr->X_fifo_1d[ph], &state_ptr->T[ph/state_ptr->num_phases]);
                        }
                        start_phase += num_phases;
                    }
                }
            }
        }
        //Compare outputs
        for(int ch=0; ch<num_y_channels; ch++) {
            for(int p=0; p<num_x_channels*state_ptr->num_phases; p++) {
                unsigned diff = vector_int32_maxdiff(
                        (int32_t*)&state_ptr->H_hat[ch][p].data[0],
                        state_ptr->H_hat[ch][p].exp,
                        (double*)&H_hat_fp[ch][p][0],
                        0,
                        (AEC_PROC_FRAME_LENGTH/2+1)*2);
                //printf("diff %d\n",diff);
                max_diff = (diff > max_diff) ? diff : max_diff;
                TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<7, diff, "H_hat diff too large.");
            }
        }
    }
    printf("max_diff %d\n",max_diff);
}
