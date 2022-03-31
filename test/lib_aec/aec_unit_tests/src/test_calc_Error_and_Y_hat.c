// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "aec_unit_tests.h"
#include <stdio.h>
#include <assert.h>
#include "aec_defines.h"
#include "aec_api.h"

#define TEST_MAIN_PHASES (5)
#define TEST_NUM_Y (1)
#define TEST_NUM_X (2)
#define TEST_SHADOW_PHASES (3)
#define NUM_BINS ((AEC_PROC_FRAME_LENGTH/2) + 1)

void calc_Error_and_Y_hat_fp(
        complex_double_t (*Error)[NUM_BINS],
        complex_double_t (*Y_hat)[NUM_BINS],
        complex_double_t (*Y)[NUM_BINS],
        complex_double_t (*H_hat_main)[TEST_NUM_X*TEST_MAIN_PHASES][NUM_BINS],
        complex_double_t (*H_hat_shad)[TEST_NUM_X*TEST_SHADOW_PHASES][NUM_BINS],
        complex_double_t (*X_fifo)[TEST_MAIN_PHASES][NUM_BINS],
        int y_channels,
        int x_channels,
        int phases,
        int is_main,
        int bypass) {
    if(bypass) {
        for(int ch=0; ch<y_channels; ch++) {
            for(int i=0; i<NUM_BINS; i++) {
                Y_hat[ch][i].re = 0.0;
                Y_hat[ch][i].im = 0.0;
                Error[ch][i].re = Y[ch][i].re;
                Error[ch][i].im = Y[ch][i].im;
            }
        }
    }
    else {
        for(int ch=0; ch<y_channels; ch++) {
            for(int i=0; i<NUM_BINS; i++) {
                Y_hat[ch][i].re = 0.0;
                Y_hat[ch][i].im = 0.0;
            }
            for(int xch=0; xch<x_channels; xch++) {
                for(int ph=0; ph<phases; ph++) {
                    if(is_main) {
                        for(int i=0; i<NUM_BINS; i++) {
                            Y_hat[ch][i].re += ((H_hat_main[ch][xch*phases + ph][i].re * X_fifo[xch][ph][i].re) - (H_hat_main[ch][xch*phases + ph][i].im * X_fifo[xch][ph][i].im));
                            Y_hat[ch][i].im += ((H_hat_main[ch][xch*phases + ph][i].re * X_fifo[xch][ph][i].im) + (H_hat_main[ch][xch*phases + ph][i].im * X_fifo[xch][ph][i].re));
                        }
                    }
                    else {
                        for(int i=0; i<NUM_BINS; i++) {
                            Y_hat[ch][i].re += ((H_hat_shad[ch][xch*phases + ph][i].re * X_fifo[xch][ph][i].re) - (H_hat_shad[ch][xch*phases + ph][i].im * X_fifo[xch][ph][i].im));                        
                            Y_hat[ch][i].im += ((H_hat_shad[ch][xch*phases + ph][i].re * X_fifo[xch][ph][i].im) + (H_hat_shad[ch][xch*phases + ph][i].im * X_fifo[xch][ph][i].re));
                        }
                    }
                }
            }
            for(int i=0; i<NUM_BINS; i++) {
                Error[ch][i].re = Y[ch][i].re - Y_hat[ch][i].re;
                Error[ch][i].im = Y[ch][i].im - Y_hat[ch][i].im;
            }
        }
    }
}

void test_calc_Error_and_Y_hat() {
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
    complex_double_t H_hat_shadow_fp[TEST_NUM_Y][TEST_NUM_X*TEST_SHADOW_PHASES][NUM_BINS];
    complex_double_t X_fifo_fp[TEST_NUM_X][TEST_MAIN_PHASES][NUM_BINS];
    complex_double_t Y_fp[TEST_NUM_Y][NUM_BINS];
    complex_double_t Y_hat_fp[TEST_NUM_Y][NUM_BINS];
    complex_double_t Error_fp[TEST_NUM_Y][NUM_BINS];

    unsigned seed = 2;
    int max_diff = 0;
    for(int iter=0; iter<(1<<12)/F; iter++) {
        int32_t new_frame[AEC_MAX_Y_CHANNELS+AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];
        unsigned is_main = pseudo_rand_uint32(&seed) % 2;
        aec_state_t *state_ptr;
        if(is_main) {
            state_ptr = &state;
        }
        else {
            state_ptr = &shadow_state;
        }
        unsigned test_l2_api = pseudo_rand_uint32(&seed) % 2;
        //printf("is_main %d, test_l2_api %d\n", is_main, test_l2_api);
        int bypass = pseudo_rand_uint32(&seed) % 2;
        state_ptr->shared_state->config_params.aec_core_conf.bypass = bypass;

        aec_frame_init(&state, &shadow_state, &new_frame[0], &new_frame[AEC_MAX_Y_CHANNELS]);        
        for(int ch=0; ch<num_y_channels; ch++) {
            //state_ptr->shared_state->Y is initialised in the y->Y fft aec_fft() call with state_ptr->shared_state->y as input. Initialising here for
            //standalone testing.
            bfp_complex_s32_init(&state_ptr->shared_state->Y[ch], (complex_s32_t*)&state_ptr->shared_state->y[ch].data[0], 0, NUM_BINS, 0);
        }
        //Generate H_hat
        for(int ch=0; ch<num_y_channels; ch++) {
            for(int ph=0; ph<num_x_channels*state_ptr->num_phases; ph++) {
                state_ptr->H_hat[ch][ph].exp = pseudo_rand_int(&seed, -31, 32);
                state_ptr->H_hat[ch][ph].hr = pseudo_rand_uint32(&seed) % 3;
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->H_hat[ch][ph].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->H_hat[ch][ph].hr;
                    state_ptr->H_hat[ch][ph].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->H_hat[ch][ph].hr;
                    if(is_main) {
                        H_hat_fp[ch][ph][i].re = ldexp(state_ptr->H_hat[ch][ph].data[i].re, state_ptr->H_hat[ch][ph].exp);
                        H_hat_fp[ch][ph][i].im = ldexp(state_ptr->H_hat[ch][ph].data[i].im, state_ptr->H_hat[ch][ph].exp);
                    }
                    else {
                        H_hat_shadow_fp[ch][ph][i].re = ldexp(state_ptr->H_hat[ch][ph].data[i].re, state_ptr->H_hat[ch][ph].exp);
                        H_hat_shadow_fp[ch][ph][i].im = ldexp(state_ptr->H_hat[ch][ph].data[i].im, state_ptr->H_hat[ch][ph].exp);
                    }
                }
            }
        }
        //Generate X_fifo, (always for number of phases in main_state)
        aec_state_t *main_state_ptr = &state;
        for(int ch=0; ch<num_x_channels; ch++) {
            for(int ph=0; ph<main_state_ptr->num_phases; ph++) {
                state_ptr->shared_state->X_fifo[ch][ph].exp = pseudo_rand_int(&seed, -31, 32);
                state_ptr->shared_state->X_fifo[ch][ph].hr = pseudo_rand_uint32(&seed) % 3;
                for(int i=0; i<NUM_BINS; i++) {
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;
                    state_ptr->shared_state->X_fifo[ch][ph].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->shared_state->X_fifo[ch][ph].hr;

                    X_fifo_fp[ch][ph][i].re = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].re, state_ptr->shared_state->X_fifo[ch][ph].exp);
                    X_fifo_fp[ch][ph][i].im = ldexp(state_ptr->shared_state->X_fifo[ch][ph].data[i].im, state_ptr->shared_state->X_fifo[ch][ph].exp);
                }
            }
        }
        //aec init only initialises the 2d Xfifo. Since we're using the 1d fifo for error computation, call aec_update_X_fifo_1d()
        //to update the 1d Fifo
        aec_update_X_fifo_1d(state_ptr);
        //Generate Y
        for(int ch=0; ch<num_y_channels; ch++) {
            state_ptr->shared_state->Y[ch].exp = pseudo_rand_int(&seed, -31, 32);
            state_ptr->shared_state->Y[ch].hr = pseudo_rand_uint32(&seed) % 3;                
            for(int i=0; i<NUM_BINS; i++) {
                state_ptr->shared_state->Y[ch].data[i].re = pseudo_rand_int32(&seed) >> state_ptr->shared_state->Y[ch].hr;
                state_ptr->shared_state->Y[ch].data[i].im = pseudo_rand_int32(&seed) >> state_ptr->shared_state->Y[ch].hr;

                Y_fp[ch][i].re = ldexp(state_ptr->shared_state->Y[ch].data[i].re, state_ptr->shared_state->Y[ch].exp);
                Y_fp[ch][i].im = ldexp(state_ptr->shared_state->Y[ch].data[i].im, state_ptr->shared_state->Y[ch].exp);
            }
        }
        //Calculate reference
        if(is_main) {
            calc_Error_and_Y_hat_fp(Error_fp, Y_hat_fp, Y_fp, H_hat_fp, NULL, X_fifo_fp, num_y_channels, num_x_channels, state_ptr->num_phases, is_main, bypass);
        }
        else {
            calc_Error_and_Y_hat_fp(Error_fp, Y_hat_fp, Y_fp, NULL, H_hat_shadow_fp, X_fifo_fp, num_y_channels, num_x_channels, state_ptr->num_phases, is_main, bypass);
        }
        //Calculate DUT
        if(!test_l2_api) {
            for(int ch=0; ch<num_y_channels; ch++) {
                aec_calc_Error_and_Y_hat(state_ptr, ch);
            }
        }
        else { //test calling l2 API directly
            #define NUM_CHUNKS_PER_Y (4) //spread each y-channel over 4 chunks
            int mapping[TEST_NUM_Y*NUM_CHUNKS_PER_Y];
            for(int i=0; i<TEST_NUM_Y*NUM_CHUNKS_PER_Y; i++) {
                mapping[i] = -1;
            }
            bfp_complex_s32_t Error_par[TEST_NUM_Y*NUM_CHUNKS_PER_Y], Y_hat_par[TEST_NUM_Y*NUM_CHUNKS_PER_Y];
            for(unsigned t=0; t<TEST_NUM_Y; t++) {
                int remaining_length = NUM_BINS;
                int start = 0;
                for(unsigned j=0; j<NUM_CHUNKS_PER_Y; j++) { 
                    unsigned ch = t;
                    unsigned length = 0;
                    unsigned start_offset = 0;
                    if((j == NUM_CHUNKS_PER_Y - 1) || (remaining_length <= 1)) {
                        length = remaining_length;
                        start_offset = start;
                        remaining_length = 0;
                    }
                    else if(remaining_length > 1) {
                        length = pseudo_rand_uint32(&seed) % remaining_length;
                        start_offset = start;
                        start += length;
                        remaining_length -= length;
                    }

                    unsigned index = (t*NUM_CHUNKS_PER_Y) + j;
                    mapping[index] = ch;
                    //printf("ych %d, chunk %d, start_offset %d, length %d, mapping[%d]=%d\n",t, j, start_offset, length, index, mapping[index]);

                    bfp_complex_s32_init(&Error_par[index], &state_ptr->Error[ch].data[start_offset], state_ptr->Error[ch].exp, length, 0);
                    Error_par[index].hr = state_ptr->Error[ch].hr;
                    bfp_complex_s32_init(&Y_hat_par[index], &state_ptr->Y_hat[ch].data[start_offset], state_ptr->Y_hat[ch].exp, length, 0);
                    Y_hat_par[index].hr = state_ptr->Y_hat[ch].hr;

                    aec_l2_calc_Error_and_Y_hat(&Error_par[index], &Y_hat_par[index], &state_ptr->shared_state->Y[ch], state_ptr->X_fifo_1d, state_ptr->H_hat[ch], num_x_channels, state_ptr->num_phases, start_offset, length, state_ptr->shared_state->config_params.aec_core_conf.bypass);
                    //printf("Error: (%d, %d), Y_hat: (%d,%d)\n", Error_par[index].exp, Error_par[index].hr, Y_hat_par[index].exp, Y_hat_par[index].hr);
                }
            }    
            //printf("\n");
            //Unify
            for(int ch=0; ch<num_y_channels; ch++) {
                int final_exp, final_hr;
                aec_l2_bfp_complex_s32_unify_exponent(Error_par, &final_exp, &final_hr, mapping, TEST_NUM_Y*NUM_CHUNKS_PER_Y, ch, 0);                
                state_ptr->Error[ch].exp = final_exp;
                state_ptr->Error[ch].hr = final_hr;
                if(state_ptr->Error[ch].exp == INT_MIN)
                {
                    assert(0);
                }
                aec_l2_bfp_complex_s32_unify_exponent(Y_hat_par, &final_exp, &final_hr, mapping, TEST_NUM_Y*NUM_CHUNKS_PER_Y, ch, 0);                
                state_ptr->Y_hat[ch].exp = final_exp;
                state_ptr->Y_hat[ch].hr = final_hr;
                if(state_ptr->Y_hat[ch].exp == INT_MIN)
                {
                    assert(0);
                }
                //printf("l2: ch %d, Error: (exp %d, hr %d), Y_hat: (exp %d, hr %d)\n",ch, state_ptr->Error[ch].exp, state_ptr->Error[ch].hr, state_ptr->Y_hat[ch].exp, state_ptr->Y_hat[ch].hr);
            }
        }

        //compare results
        //printf("iter = %d\n",iter);
        for(int ch=0; ch<num_y_channels; ch++) {
            int32_t *dut_Error_ptr = (int32_t*)&state_ptr->Error[ch].data[0];
            double *ref_Error_ptr = (double*)&Error_fp[ch][0];

            int32_t *dut_Y_hat_ptr = (int32_t*)&state_ptr->Y_hat[ch].data[0];
            double *ref_Y_hat_ptr = (double*)&Y_hat_fp[ch][0];
            for(int i=0; i<NUM_BINS*2; i++) {
                //Error
                int32_t diff = double_to_int32(ref_Error_ptr[i], state_ptr->Error[ch].exp) - dut_Error_ptr[i];
                diff = (diff < 0) ? -diff : diff;
                if(diff > max_diff) max_diff = diff;
                TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<4, max_diff, "Error diff too large.");
                //Y_hat
                diff = double_to_int32(ref_Y_hat_ptr[i], state_ptr->Y_hat[ch].exp) - dut_Y_hat_ptr[i];
                diff = (diff < 0) ? -diff : diff;
                if(diff > max_diff) max_diff = diff;
                TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE(1<<4, max_diff, "Y_hat diff too large.");
            }            
        }
        
    }
    printf("max_diff = %d\n",max_diff);
}
