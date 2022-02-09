// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// XMOS DSP Library - Example to use FFT and inverse FFT

#include <stdio.h>
#include <stdlib.h>
//#include <xs1.h>
#include <xclib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "ai.h"
#include "vad_api.h"
#include "vad_parameters.h"
#include "vad_nn_coefficients.h"
#include "vad_mel_scale.h"
#include "vad_normalisation.h"
#include "vad_window.h"
#include "vad_mel.h"
#include "dsp.h"

#include "xs3_math.h"
#include <limits.h>

int32_t vad_spectral_centroid_Hz(complex_s32_t * p, uint32_t N) {
    uint64_t sum = 0, tav = 0;
    int logN = 31 - clz(N);
    for(int i = 0; i < N; i++) {
        uint32_t energy = (p[i].re * (int64_t) p[i].re + 
                           p[i].im * (int64_t) p[i].im) >> 32; // 1 bit headroom
        sum += (energy * i * 8000ULL) >> logN;
        tav += energy;
    }
    uint64_t div = tav;
    if (div == 0) return sum;
    return sum / div;
}

int32_t vad_spectral_spread_Hz(complex_s32_t * p, uint32_t N,
                               int32_t spectral_centroid) {
    int logN = 31 - clz(N);
    uint64_t sum = 0, tav = 0;
    uint32_t headroom = 0;
    if (logN > 5) {
        headroom = logN - 5;
    }
    for(int i = 0; i < N; i++) {
        uint32_t energy = (p[i].re * (int64_t) p[i].re + 
                           p[i].im * (int64_t) p[i].im) >> 32; // 1 bit headroom
        int32_t c =  ((i * 8000) >> logN) - spectral_centroid;
        uint32_t variance = c * c;                               
        sum += (energy * (uint64_t) variance) >> headroom;
        tav += energy;
    }
    uint64_t div = tav >> headroom;
    if (div == 0) return sum;
    return sum / div;
}


void vad_fc_layer(  int64_t output[], const size_t num_out,
                    int32_t input[], const size_t num_in,
                    int32_t weights[]){

    const int num_bias = 1; //One bias value before the weights in the weights array

    for(int o = 0; o < num_out; o++) {
        int bias_idx = (num_bias + num_in) * o;
        int64_t bias = ((int64_t)weights[bias_idx] << AI_NN_VALUE_Q);

        int weights_idx = bias_idx + num_bias;
        int64_t dot = xs3_vect_s32_dot( &weights[weights_idx],
                                        input,
                                        num_in,
                                        0, 0);

        int64_t node = (dot << 30) + bias; //30 bit shift is an atrefact of xs3_vect_s32_dot
        output[o] = node;
    }
}


void vad_relu(int32_t activated[], int64_t raw_layer[], const size_t N){
    for(int i=0; i<N; i++){
        const int64_t max = 0x7fffffffLL << AI_NN_WEIGHT_Q;
        int64_t clamped_relu = raw_layer[i];
        if (clamped_relu > max){
            clamped_relu = max;
        }else if(clamped_relu < 0){
            clamped_relu = 0;
        }
        activated[i] = clamped_relu >> AI_NN_WEIGHT_Q;
    }
}

void vad_reduce_sigmoid(int32_t out_data[],
                       const int64_t in_data[],
                       uint32_t N) {
    for(uint32_t i = 0; i < N; i++) {
        int32_t shift = AI_NN_OUTPUT_Q-24;
        int64_t x = in_data[i];
        long long max = 0x7fffffffLL << shift;
        if (x > max) x = max;
        if (x < -max-1) x = -max-1;
        int32_t in_32 = x >> shift;
        out_data[i] = dsp_math_logistics_fast(in_32);
    }
}


void vad_form_frame(int32_t * current, const int32_t * input, int32_t * prev){

    memcpy(current, prev, (VAD_PROC_FRAME_LENGTH - VAD_FRAME_ADVANCE) * sizeof(int32_t));
    memcpy(&current[VAD_PROC_FRAME_LENGTH - VAD_FRAME_ADVANCE], input, VAD_FRAME_ADVANCE * sizeof(int32_t));

    memcpy(prev, &prev[VAD_FRAME_ADVANCE], (VAD_PROC_FRAME_LENGTH - (2 * VAD_FRAME_ADVANCE)) * sizeof(int32_t));
    memcpy(&prev[VAD_PROC_FRAME_LENGTH - (2 * VAD_FRAME_ADVANCE)], input, VAD_FRAME_ADVANCE * sizeof(int32_t));
}

void vad_init(vad_state_t *state) {
    memset(state, 0, sizeof(vad_state_t));
}

#define PRINT_ME 0
#define PRINT_ALL 0

int32_t vad_probability_voice(const int32_t input[VAD_FRAME_ADVANCE],
                            vad_state_t * state){
    int32_t DWORD_ALIGNED curr[VAD_PROC_FRAME_LENGTH];
    int32_t mel[VAD_N_MEL_SCALE + 1];
    int32_t dct_input[VAD_N_DCT];
    int32_t dct_output[VAD_N_DCT];
    int32_t features[VAD_N_FEATURES_PER_FRAME];
    int32_t nn_features[VAD_N_FEATURES];

    int64_t hidden_nodes_full[N_VAD_HIDDEN];
    int32_t hidden_nodes_normal[N_VAD_HIDDEN];
    
    int64_t outputs_nodes_full[N_VAD_OUTPUTS];
    int32_t outputs_nodes_normal[N_VAD_OUTPUTS];


#if PRINT_ME && PRINT_ALL
    printf("INP ");
    for(int i = 0; i < VAD_WINDOW_LENGTH; i++) {
        printf("%d ", input[i]);
    }
    printf("\n");
#endif

    //512 packing
    vad_form_frame(curr, input, state->prev_frame);

    for(int i = 0; i < VAD_PROC_FRAME_LENGTH; i++) {
        //input[i].re = (time_domain_input[i] * (int64_t)vad_window[i]) >> 31;
        //input[i].im = 0;
        curr[i] = (curr[i] * (int64_t)vad_window[i]) >> 31;
    }

    //bfp_s32_mul(frame, window);

#if PRINT_ME && PRINT_ALL
    printf("WINDOWED ");
    for(int i = 0; i < VAD_WINDOW_LENGTH; i++) {
        printf("%d ", curr[i]);
    }
    printf("\n");
#endif

#if 0
    int headroom = dsp_bfp_cls(input, VAD_WINDOW_LENGTH)-1;
    dsp_bfp_shl(input, VAD_WINDOW_LENGTH, headroom);

    // First compute frequency domain: input e [-2^31..2^31], output div by N
    dsp_fft_bit_reverse(input, VAD_WINDOW_LENGTH);         
    dsp_fft_forward(input, VAD_WINDOW_LENGTH, VAD_DSP_SINE_WINDOW_LENGTH);
#else    
    //WORK IN PROGRESS - THIS DOES NOT WORK YET 
    complex_s32_t* curr_fd = (complex_s32_t*)curr;
    exponent_t x_exp = -31;
    headroom_t hr = xs3_vect_s32_headroom(curr, VAD_PROC_FRAME_LENGTH);

    right_shift_t x_shr = 2 - hr;
    xs3_vect_s32_shl(curr, curr, VAD_PROC_FRAME_LENGTH, -x_shr);
    hr += x_shr; x_exp += x_shr;

    xs3_fft_index_bit_reversal(curr_fd, VAD_PROC_FRAME_BINS);
    xs3_fft_dit_forward(curr_fd, VAD_PROC_FRAME_BINS, &hr, &x_exp);
    xs3_fft_mono_adjust(curr_fd, VAD_PROC_FRAME_LENGTH, 0);
#endif


#if PRINT_ME && PRINT_ALL
    printf("SPECTRAL ");
    for(int i = 0; i < VAD_WINDOW_LENGTH/2; i++) {
        printf("%d %d     ", curr_fd[i].re, curr_fd[i].im);
    }
    printf("\n");
#endif
    // Compute spectral centroid and spread; in 32.0 format
    int32_t spectral_centroid = vad_spectral_centroid_Hz(curr_fd, VAD_PROC_FRAME_BINS);
    
    int32_t spectral_spread   = vad_spectral_spread_Hz(curr_fd, VAD_PROC_FRAME_BINS, spectral_centroid);

    // Compute MEL frequencies; 41 of them (including first one), compensate
    // for 2 * logN bits that are lost in the FFT.
    
    vad_mel_compute(mel, VAD_N_MEL_SCALE + 1, curr_fd, VAD_PROC_FRAME_BINS, vad_mel_table24_512, 2 * (VAD_LOG_WINDOW_LENGTH + (-31 - x_exp)));
    
    // Mel coefficients are in 8.24; make them in 16.16 format for headroom
    for(int i = 0; i < VAD_N_MEL_SCALE; i++) {
        dct_input[i] = (mel[i + 1] >> 8);   // create headroom.
    }
    
#if PRINT_ME
    printf("MEL ");
    for(int i = 0; i < 24; i++) {
        printf("%5.2f ", dct_input[i]/65536.0);
    }
    printf("\n");
#endif
    
    // And take DCT
#if (VAD_N_DCT != 24)
    #error VAD_N_DCT must be 24
#endif

    dsp_dct_forward24(dct_output, dct_input);
    
    // Python multiplies DCT by 2.
    for(int i = 0; i < VAD_N_DCT; i++) {
        dct_output[i] *= 2;
    }

#if PRINT_ME
    printf("DCT ");
    for(int i = 0; i < 24; i++) {
        printf("%5.2f ", dct_output[i]/65536.0);
    }
    printf("\n");
#endif

    features[0] = spectral_centroid;
    features[1] = spectral_spread;
    for(int i = 0; i < VAD_N_FEATURES_PER_FRAME-3; i++) {
        features[i+2] = dct_output[i+1];
    }
    features[VAD_N_FEATURES_PER_FRAME-1] = dct_output[0] - state->old_features[VAD_N_OLD_FEATURES - 1];
    for(int i = VAD_N_OLD_FEATURES - 2;
            i >= VAD_N_OLD_FEATURES - VAD_FRAME_STRIDE;
            i--) {
        state->old_features[i+1] = state->old_features[i];
    }
    state->old_features[VAD_N_OLD_FEATURES-VAD_FRAME_STRIDE] = dct_output[0] ;
#if PRINT_ME
    printf("Old DCT0: ");
    for(int i = VAD_N_OLD_FEATURES - VAD_FRAME_STRIDE;
            i < VAD_N_OLD_FEATURES ;
            i++) {
        printf("%5.2f ", old_features[i]/(65536.0));
    }
    printf("\nNormalised: ");
    for(int i = 0; i < VAD_N_FEATURES_PER_FRAME; i++) {
        printf("%5.2f ", features[i]/(i < 2 ? 1.0: 65536.0));
    }
    printf("\n");
#endif
    features[VAD_N_FEATURES_PER_FRAME-1] = 
        (((long long)(features[VAD_N_FEATURES_PER_FRAME-1] - vad_mus[VAD_N_FEATURES_PER_FRAME-1])) << 24) / vad_sigmas[VAD_N_FEATURES_PER_FRAME-1];

    for(int i = 0; i < VAD_N_FEATURES_PER_FRAME-1; i++) {
        features[i] = (((long long)(features[i] - vad_mus[i])) << 24) / vad_sigmas[i];
    }

    // First copy features over into input for NN, both current and old data
    for(int i = 0; i < VAD_N_FEATURES_PER_FRAME; i++) {
        nn_features[i] = features[i];
    }
    int oindex = VAD_N_FEATURES_PER_FRAME;
    // Old data has to be picked up strided
    for(int i = (VAD_FRAME_STRIDE-1) * VAD_N_FEATURES_PER_FRAME; i < VAD_N_OLD_FEATURES-1-VAD_FRAME_STRIDE; i += VAD_FRAME_STRIDE * VAD_N_FEATURES_PER_FRAME) {
        for(int j = 0; j < VAD_N_FEATURES_PER_FRAME; j++) {
            nn_features[oindex] = state->old_features[i + j];
            oindex++;
        }
    }

    // Now compute dMFCC0 for frames as dMFCC0[n] = MFCC0[n] - MFCC0[n-3]
    for(int i = VAD_N_FEATURES_PER_FRAME-1;
            i < VAD_N_FEATURES - 2;
            i += VAD_N_FEATURES_PER_FRAME) {
  //      nn_features[i] -= nn_features[i+VAD_N_FEATURES_PER_FRAME];        
    }
  //  nn_features[VAD_N_FEATURES - 1] -= old_features[VAD_N_OLD_FEATURES - 1];

    // Now copy old data back; keeping an extra dMFCC0.
    //old_features[VAD_N_OLD_FEATURES-1] = old_features[VAD_N_OLD_FEATURES-2];
    for(int i = VAD_N_OLD_FEATURES - VAD_N_FEATURES_PER_FRAME-1-VAD_FRAME_STRIDE; i >= 0; i--) {
        state->old_features[i+VAD_N_FEATURES_PER_FRAME] = state->old_features[i];
    }
    for(int i = 0; i < VAD_N_FEATURES_PER_FRAME; i++) {
        state->old_features[i] = features[i];
    }

#if PRINT_ME
    for(int i = 0; i < VAD_N_FEATURES; i++) {
        if (i % 26 == 0) printf("Features[%d..%d] ", i, i+25);
        printf("%5.2f ", nn_features[i]/65536.0/256.0);
        if (i % 26 == 25) printf("\n");
    }
    printf("\n");
#endif

    nn_layer_fc(hidden_nodes_full, N_VAD_HIDDEN,
                nn_features,       N_VAD_INPUTS,
                hidden_coeffs);

    int64_t hidden_nodes_xs3m[N_VAD_HIDDEN];


    vad_fc_layer(   hidden_nodes_xs3m, N_VAD_HIDDEN,
                    nn_features, N_VAD_INPUTS,
                    hidden_coeffs);


    for(int o = 0; o < N_VAD_HIDDEN; o++) {
        // printf("%d xs3m: %lld, (ref: %lld)\n", o, hidden_nodes_xs3m[o], hidden_nodes_full[o]);
    }


    int32_t activated_l1[N_VAD_HIDDEN];
    vad_relu(activated_l1, hidden_nodes_xs3m, N_VAD_HIDDEN);

    nn_reduce_relu(hidden_nodes_normal,
                   hidden_nodes_full,
                   N_VAD_HIDDEN);


    for(int o = 0; o < N_VAD_HIDDEN; o++) {
        // printf("hidden_nodes_normal %d xs3m: %ld, (ref: %ld)\n", o, activated_l1[o], hidden_nodes_normal[o]);
    }

    int64_t output_nodes_xs3m[N_VAD_OUTPUTS];


    nn_layer_fc(outputs_nodes_full,    N_VAD_OUTPUTS,
                hidden_nodes_normal, N_VAD_HIDDEN,
                outputs_coeffs);

    vad_fc_layer(output_nodes_xs3m, N_VAD_OUTPUTS,
                activated_l1, N_VAD_HIDDEN,
                outputs_coeffs);


    for(int o = 0; o < N_VAD_OUTPUTS; o++) {
        // printf("outputs_nodes %d xs3m: %lld, (ref: %lld)\n", o, output_nodes_xs3m[o], outputs_nodes_full[o]);
    }


    nn_reduce_sigmoid(outputs_nodes_normal,
                      outputs_nodes_full,
                      N_VAD_OUTPUTS);
    
    int32_t output_nodes_activated[N_VAD_OUTPUTS];
    vad_reduce_sigmoid(output_nodes_activated, output_nodes_xs3m, N_VAD_OUTPUTS);

    for(int o = 0; o < N_VAD_OUTPUTS; o++) {
        // printf("outputs_nodes_activated %d xs3m: %ld, (ref: %ld)\n", o, output_nodes_activated[o], outputs_nodes_normal[o]);
    }
    // printf("**********\n");


    for(int i = 0; i < N_VAD_HIDDEN; i++) {
//        printf("Hidden node %d pre RELU %7.4f, post RELU %7.4f\n", i, hidden_nodes_full[i]/(float)(1LL << 48), hidden_nodes_normal[i]/(float)(1<<24));
    }
    for(int i = 0; i < N_VAD_OUTPUTS; i++) {
//        printf("Final Node %d pre Sigmoid %7.4f, post Sigmoid %7.4f %08x   ", i, outputs_nodes_full[i]/(float)(1LL << 48), outputs_nodes_normal[i]/(float)(1<<24), outputs_nodes_normal[i] );
    }
//    return ((outputs_nodes_normal[0] >> 16)*100) >> 8;
    return outputs_nodes_normal[0]>>16;
}

