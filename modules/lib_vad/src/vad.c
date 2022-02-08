// Copyright 2015-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// XMOS DSP Library - Example to use FFT and inverse FFT

#include <stdio.h>
#include <stdlib.h>
#include <xs1.h>
#include <xclib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
// #include "ai.h"
#include "vad.h"
#include "vad_parameters.h"
#include "vad_nn_coefficients.h"
#include "vad_mel_scale.h"
#include "vad_normalisation.h"
#include "vad_window.h"
#include "vad_mel.h"
#include "dsp.h"

#include "xs3_math.h"

int32_t vad_spectral_centroid_Hz(dsp_complex_t p[], uint32_t N) {
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

int32_t vad_spectral_spread_Hz(dsp_complex_t p[], uint32_t N,
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

void vad_init(vad_state_t *state) {
    memset(state, 0, sizeof(vad_state_t));
}

#define PRINT_ME 0
#define PRINT_ALL 0

int32_t vad_probabiity_voice(int32_t time_domain_input[VAD_WINDOW_LENGTH],
        vad_state_t *state) {
    dsp_complex_t input[VAD_WINDOW_LENGTH];
    int32_t mel[VAD_N_MEL_SCALE+1];
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
        printf("%d ", time_domain_input[i].re);
    }
    printf("\n");
#endif

    for(int i = 0; i < VAD_WINDOW_LENGTH; i++) {
        input[i].re = (time_domain_input[i] * (int64_t)vad_window[i]) >> 31;
        input[i].im = 0;
    }

#if PRINT_ME && PRINT_ALL
    printf("WINDOWED ");
    for(int i = 0; i < VAD_WINDOW_LENGTH; i++) {
        printf("%d ", input[i].re);
    }
    printf("\n");
#endif

#if 1
    int headroom = dsp_bfp_cls(input, VAD_WINDOW_LENGTH)-1;
    dsp_bfp_shl(input, VAD_WINDOW_LENGTH, headroom);

    // First compute frequency domain: input e [-2^31..2^31], output div by N
    dsp_fft_bit_reverse(input, VAD_WINDOW_LENGTH);         
    dsp_fft_forward(input, VAD_WINDOW_LENGTH, VAD_DSP_SINE_WINDOW_LENGTH);
#else    
    //WORK IN PROGRESS - THIS DOES NOT WORK YET 
    complex_s32_t* input_fd = (complex_s32_t*)input;
    exponent_t x_exp = -31;
    headroom_t hr = xs3_vect_s32_headroom(input, VAD_WINDOW_LENGTH);
    xs3_fft_index_bit_reversal(input_fd, VAD_WINDOW_LENGTH);
    xs3_fft_dit_forward(input_fd, VAD_WINDOW_LENGTH/2, &hr, &x_exp);
    xs3_fft_mono_adjust(input_fd, VAD_WINDOW_LENGTH, 0);
#endif


#if PRINT_ME && PRINT_ALL
    printf("SPECTRAL ");
    for(int i = 0; i < VAD_WINDOW_LENGTH/2; i++) {
        printf("%d %d     ", input[i].re, input[i].im);
    }
    printf("\n");
#endif
    // Compute spectral centroid and spread; in 32.0 format
    int32_t spectral_centroid = vad_spectral_centroid_Hz(input, VAD_WINDOW_LENGTH/2);
    
    int32_t spectral_spread   = vad_spectral_spread_Hz(input, VAD_WINDOW_LENGTH/2, spectral_centroid);

    // Compute MEL frequencies; 41 of them (including first one), compensate
    // for 2 * logN bits that are lost in the FFT.
    
    vad_mel_compute(mel, VAD_N_MEL_SCALE+1, input, VAD_WINDOW_LENGTH/2, vad_mel_table24_512, 2*VAD_LOG_WINDOW_LENGTH-2*headroom);
    
    // Mel coefficients are in 8.24; make them in 16.16 format for headroom
    for(int i = 0; i < VAD_N_MEL_SCALE; i++) {
        dct_input[i] = (mel[i+1] >> 8);   // create headroom.
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

    // nn_layer_fc(hidden_nodes_full, N_VAD_HIDDEN,
    //             nn_features,       N_VAD_INPUTS,
    //             hidden_coeffs);
    // nn_reduce_relu(hidden_nodes_normal,
    //                hidden_nodes_full,
    //                N_VAD_HIDDEN);
    // nn_layer_fc(outputs_nodes_full,    N_VAD_OUTPUTS,
    //             hidden_nodes_normal, N_VAD_HIDDEN,
    //             outputs_coeffs);
    // nn_reduce_sigmoid(outputs_nodes_normal,
    //                   outputs_nodes_full,
    //                   N_VAD_OUTPUTS);
    
    for(int i = 0; i < N_VAD_HIDDEN; i++) {
//        printf("Hidden node %d pre RELU %7.4f, post RELU %7.4f\n", i, hidden_nodes_full[i]/(float)(1LL << 48), hidden_nodes_normal[i]/(float)(1<<24));
    }
    for(int i = 0; i < N_VAD_OUTPUTS; i++) {
//        printf("Final Node %d pre Sigmoid %7.4f, post Sigmoid %7.4f %08x   ", i, outputs_nodes_full[i]/(float)(1LL << 48), outputs_nodes_normal[i]/(float)(1<<24), outputs_nodes_normal[i] );
    }
//    return ((outputs_nodes_normal[0] >> 16)*100) >> 8;
    return outputs_nodes_normal[0]>>16;
}

