// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "vad_api.h"
#include "vad_parameters.h"
#include "vad_nn_coefficients.h"
#include "vad_mel_scale.h"
#include "vad_normalisation.h"
#include "vad_window.h"
#include "vad_mel.h"
#include "vad_dct.h"
#include "xs3_math.h"
#include "vad_helpers.h"

#if X86_BUILD
#define clz(v)  clz_sim(v)
#endif

headroom_t vad_xs3_math_fft(int32_t * curr, int nq){

    complex_s32_t* curr_fd = (complex_s32_t*)curr;

    //Ensure we have 2b headroom as required by xs3_math
    exponent_t x_exp = -31;
    headroom_t original_hr = xs3_vect_s32_headroom(curr, VAD_PROC_FRAME_LENGTH);
    headroom_t xs3_output_hr = original_hr;

    right_shift_t x_shr = 2 - original_hr;
    xs3_vect_s32_shl(curr, curr, VAD_PROC_FRAME_LENGTH, -x_shr);
    xs3_output_hr += x_shr; x_exp += x_shr;

    xs3_fft_index_bit_reversal(curr_fd, VAD_PROC_FRAME_BINS);
    xs3_fft_dit_forward(curr_fd, VAD_PROC_FRAME_BINS, &xs3_output_hr, &x_exp);
    xs3_fft_mono_adjust(curr_fd, VAD_PROC_FRAME_LENGTH, 0);

    if(nq){
        curr_fd[VAD_PROC_FRAME_BINS].re = curr_fd[0].im;
        curr_fd[0].im = 0;
        curr_fd[VAD_PROC_FRAME_BINS].im = 0;
    }
    //Now adjust exponent to match output of lib_dsp
    //This is tedious as BFP result is same but we need identical matnissas
    headroom_t lib_dsp_exp = VAD_LOG_WINDOW_LENGTH - original_hr;
    headroom_t xs3_math_exp = x_exp + 31;
    headroom_t exp_adjust = lib_dsp_exp - xs3_math_exp; 
    xs3_vect_s32_shr(curr, curr, VAD_PROC_FRAME_LENGTH, exp_adjust);
    x_exp += exp_adjust;

    return original_hr;
}

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
                    const int32_t input[], const size_t num_in,
                    const int32_t weights[]){

    const int num_bias = 1; //One bias value before the weights in the weights array

    for(int o = 0; o < num_out; o++) {
        int bias_idx = (num_bias + num_in) * o;
        int64_t bias = ((int64_t)weights[bias_idx] << VAD_AI_NN_VALUE_Q);

        int weights_idx = bias_idx + num_bias;
        int64_t dot = xs3_vect_s32_dot( &weights[weights_idx],
                                        input,
                                        num_in,
                                        0, 0);

        int64_t node = (dot << 30) + bias; //30 bit shift is an atrefact of xs3_vect_s32_dot
        output[o] = node;
    }
}


void vad_reduce_relu(int32_t activated[], int64_t raw_layer[], const size_t N){
    for(int i=0; i<N; i++){
        const int64_t max = 0x7fffffffLL << VAD_AI_NN_WEIGHT_Q;
        int64_t clamped_relu = raw_layer[i];
        if (clamped_relu > max){
            clamped_relu = max;
        }else if(clamped_relu < 0){
            clamped_relu = 0;
        }
        activated[i] = clamped_relu >> VAD_AI_NN_WEIGHT_Q;
    }
}

void vad_reduce_sigmoid(int32_t out_data[],
                       const int64_t in_data[],
                       const size_t N) {
    for(uint32_t i = 0; i < N; i++) {
        int32_t shift = VAD_AI_NN_OUTPUT_Q-24;
        int64_t x = in_data[i];
        long long max = 0x7fffffffLL << shift;
        if (x > max) x = max;
        if (x < -max-1) x = -max-1;
        int32_t in_32 = x >> shift;
        out_data[i] = vad_math_logistics_fast(in_32);
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
    int32_t DWORD_ALIGNED curr[VAD_PROC_FRAME_LENGTH + 2];
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

    //240 -> 512 packing
    vad_form_frame(curr, input, state->prev_frame);

    for(int i = 0; i < VAD_PROC_FRAME_LENGTH; i++) {
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

    headroom_t headroom = vad_xs3_math_fft(curr, 1);
    complex_s32_t* curr_fd = (complex_s32_t*)curr;


#if PRINT_ME && PRINT_ALL
    printf("NEW SPECTRAL ");
    for(int i = 0; i < VAD_WINDOW_LENGTH/2; i++) {
        printf("%ld %ld     ", input[i].re, input[i].im);
    }
    printf("\n");
#endif

    // Compute spectral centroid and spread; in 32.0 format
    int32_t spectral_centroid = vad_spectral_centroid_Hz(curr_fd, VAD_PROC_FRAME_BINS);
    
    int32_t spectral_spread   = vad_spectral_spread_Hz(curr_fd, VAD_PROC_FRAME_BINS, spectral_centroid);

    // Compute MEL frequencies; 41 of them (including first one), compensate
    // for 2 * logN bits that are lost in the FFT.
    vad_mel_compute_new(mel, VAD_N_MEL_SCALE+1, curr_fd, VAD_WINDOW_LENGTH/2, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);

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

    vad_dct_forward24(dct_output, dct_input);

    // Python multiplies DCT by 2 so match model
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

    vad_fc_layer(   hidden_nodes_full, N_VAD_HIDDEN,
                    nn_features, N_VAD_INPUTS,
                    hidden_coeffs);

    vad_reduce_relu(hidden_nodes_normal, hidden_nodes_full, N_VAD_HIDDEN);

    vad_fc_layer(outputs_nodes_full, N_VAD_OUTPUTS,
                hidden_nodes_normal, N_VAD_HIDDEN,
                outputs_coeffs);

    vad_reduce_sigmoid(outputs_nodes_normal, outputs_nodes_full, N_VAD_OUTPUTS);


#if PRINT_ME
    for(int i = 0; i < N_VAD_HIDDEN; i++) {
       printf("Hidden node %d pre RELU %7.4f, post RELU %7.4f\n", i, hidden_nodes_full[i]/(float)(1LL << 48), hidden_nodes_normal[i]/(float)(1<<24));
    }
    for(int i = 0; i < N_VAD_OUTPUTS; i++) {
       printf("Final Node %d pre Sigmoid %7.4f, post Sigmoid %7.4f %08x   ", i, outputs_nodes_full[i]/(float)(1LL << 48), outputs_nodes_normal[i]/(float)(1<<24), outputs_nodes_normal[i] );
    }
#endif

    //Note reduce sigmoid outputs maximum 0.999999 in 8.24 format so this cannot overflow 255
    return outputs_nodes_normal[0]>>16;
}
