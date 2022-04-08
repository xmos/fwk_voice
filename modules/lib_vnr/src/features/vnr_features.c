// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "vnr_features_api.h"
#include "mel_filter_512_24_compact.h"

static int framenum=0;
void vnr_input_state_init(vnr_input_state_t *input_state) {
    memset(input_state, 0, sizeof(vnr_input_state_t));
}

void vnr_form_input_frame(vnr_input_state_t *input_state, int32_t *x_data, const int32_t *new_x_frame) {
    // Update current frame
    memcpy(x_data, input_state->prev_input_samples, (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
    memcpy(&x_data[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE], new_x_frame, VNR_FRAME_ADVANCE*sizeof(int32_t));

    // Update previous samples
    memcpy(input_state->prev_input_samples, &x_data[VNR_FRAME_ADVANCE], (VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE)*sizeof(int32_t));
}

void vnr_forward_fft(bfp_complex_s32_t *X, int32_t *x_data) {
    bfp_s32_t x;
    bfp_s32_init(&x, x_data, VNR_INPUT_EXP, VNR_PROC_FRAME_LENGTH, 1);

    bfp_complex_s32_t *temp = bfp_fft_forward_mono(&x);
#if HEADROOM_CHECK
    headroom_t reported_hr = temp->hr;
    headroom_t actual_hr = bfp_complex_s32_headroom(temp);
    if(reported_hr != actual_hr) {
        printf("ERROR: bfp_fft_forward_mono(), reported hr %d, actual %d\n",reported_hr, actual_hr);
        assert(0);
    }
#endif
    //printf("post fft hr = reported %d, actual %d\n",temp->hr, bfp_complex_s32_headroom(temp));
    temp->hr = bfp_complex_s32_headroom(temp); // bfp_fft_forward_mono() reported headroom is sometimes incorrect
    bfp_fft_unpack_mono(temp);
    memcpy(X, temp, sizeof(bfp_complex_s32_t));
}

void vnr_mel_compute(float_s32_t *filter_output, bfp_complex_s32_t *X) {
    // Calculate squared magnitude spectrum
    int32_t DWORD_ALIGNED squared_mag_data[VNR_FD_FRAME_LENGTH];
    bfp_s32_t squared_mag;
    bfp_s32_init(&squared_mag, squared_mag_data, 0, X->length, 0);
    bfp_complex_s32_squared_mag(&squared_mag, X);
    /*if(framenum==66) {
        for(int i=0; i<20; i++) {
            printf("X[%d] bfp = (re %ld im %ld), (exp %d, hr %d), sq_mag = (%ld, exp %d, hr %d)\n", i, X->data[i].re, X->data[i].im, X->exp, X->hr, squared_mag.data[i], squared_mag.exp, squared_mag.hr);
            printf("X[%d] = (%.6f. %.6f), sq_mag = %.6f\n",i, ldexp(X->data[i].re, X->exp), ldexp(X->data[i].im, X->exp), ldexp(squared_mag.data[i], squared_mag.exp) );
        }
    }*/
#if HEADROOM_CHECK
    headroom_t reported_hr = squared_mag.hr;
    headroom_t actual_hr = bfp_s32_headroom(&squared_mag);
    if(reported_hr != actual_hr) {
        printf("ERROR: bfp_complex_s32_squared_mag(), reported hr %d, actual %d\n",reported_hr, actual_hr);
        assert(0);
    }
#endif

    // Mel filtering
    unsigned num_bands = AUDIO_FEATURES_NUM_MELS + 1; //Extra band for the 2nd half of the last filter  
    unsigned num_even_filters = num_bands / 2;
    unsigned num_odd_filters = AUDIO_FEATURES_NUM_MELS - num_even_filters;
    int filter_exp = -(31-AUDIO_FEATURES_MEL_HEADROOM_BITS);
    int filter_hr = AUDIO_FEATURES_MEL_HEADROOM_BITS;
    for(unsigned i=0; i<num_even_filters; i++) {
        unsigned filter_start = mel_filter_512_24_compact_start_bins[2*i];
        unsigned filter_length = mel_filter_512_24_compact_start_bins[2*(i+1)] - filter_start;
        //Create the bfp for spectrum subset
        bfp_s32_t spect_subset;
        bfp_s32_init(&spect_subset, &squared_mag.data[filter_start], squared_mag.exp, filter_length, 0);
        spect_subset.hr = squared_mag.hr;

        //Create BFP for the filter
        bfp_s32_t filter_subset;
        bfp_s32_init(&filter_subset, &mel_filter_512_24_compact_q31[filter_start], filter_exp, filter_length, 0);
        filter_subset.hr = filter_hr;
        
        filter_output[2*i] = float_s64_to_float_s32(bfp_s32_dot(&spect_subset, &filter_subset));
    }

    bfp_s32_t filter;
    bfp_s32_init(&filter, &mel_filter_512_24_compact_q31[0], filter_exp, AUDIO_FEATURES_NUM_BINS, 0);

    int32_t odd_band_filters_data[AUDIO_FEATURES_NUM_BINS];
    bfp_s32_t odd_band_filters;
    bfp_s32_init(&odd_band_filters, odd_band_filters_data, 0, AUDIO_FEATURES_NUM_BINS, HR_S32(AUDIO_FEATURES_MEL_MAX));
    bfp_s32_set(&odd_band_filters, AUDIO_FEATURES_MEL_MAX, filter_exp);
    bfp_s32_sub(&odd_band_filters, &odd_band_filters, &filter);

    for(unsigned i=0; i<num_odd_filters; i++) {
        unsigned filter_start = mel_filter_512_24_compact_start_bins[(2*i) + 1];
        unsigned filter_length = mel_filter_512_24_compact_start_bins[(2*(i+1)) + 1] - filter_start;
        //Create the bfp for spectrum subset
        bfp_s32_t spect_subset;
        bfp_s32_init(&spect_subset, &squared_mag.data[filter_start], squared_mag.exp, filter_length, 0);
        spect_subset.hr = squared_mag.hr;

        //Create BFP for the filter
        bfp_s32_t filter_subset;
        bfp_s32_init(&filter_subset, &odd_band_filters.data[filter_start], odd_band_filters.exp, filter_length, 0);
        filter_subset.hr = odd_band_filters.hr;

        filter_output[(2*i)+1] = float_s64_to_float_s32(bfp_s32_dot(&spect_subset, &filter_subset));
    }
    framenum++;
}

void vnr_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length) {
    for(unsigned i=0; i<length; i++) {
        output_q24[i] = float_s32_to_fixed_q24_log2(input[i]);
    }
}

#define LOOKUP_PRECISION 8
#define MEL_PRECISION 24
static const int lookup[33] = {
    0,    11,  22,  33,  43,  53,  63,  73,
    82,   91, 100, 109, 117, 125, 134, 141,
    149, 157, 164, 172, 179, 186, 193, 200,
    206, 213, 219, 225, 232, 238, 244, 250,
    256
};



int lookup_small_log2_linear_new(uint32_t x) {
    int mask_bits = 26;
    int mask = (1 << mask_bits) - 1;
    int y = (x >> mask_bits) - 32;
    int y1 = y + 1;
    int v0 = lookup[y], v1 = lookup[y1];
    int f1 = x & mask;
    int f0 = mask + 1 - f1;
    return (v0 * (uint64_t) f0 + v1 * (uint64_t) f1) >> (mask_bits - (MEL_PRECISION - LOOKUP_PRECISION));
}

fixed_s32_t float_s32_to_fixed_q24_log2(float_s32_t x) {
    headroom_t hr = HR_S32(x.mant);
    hr = hr + 1;
    uint32_t x_mant = 0;
    exponent_t x_exp = 0;
    x_mant = (uint32_t)x.mant << hr;
    x_exp = x.exp - hr;

    //Handle case for zero mantissa which is possible with float_s32_t
    if (x_mant == 0) return INT_MIN;

    int32_t number_of_bits = 31 + x_exp;
    number_of_bits <<= MEL_PRECISION;

    uint32_t log2 = lookup_small_log2_linear_new(x_mant) + number_of_bits;
    return log2;
}

void vnr_extract_features(vnr_input_state_t *input_state, const int32_t *new_x_frame, /*for debug*/ bfp_complex_s32_t *fft_output, bfp_s32_t *squared_mag_out)
{
    int32_t DWORD_ALIGNED x_data[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
    vnr_form_input_frame(input_state, x_data, new_x_frame);
    
    bfp_complex_s32_t X;
    vnr_forward_fft(&X, x_data);

    //For debug.
    fft_output->hr = X.hr;
    fft_output->exp = X.exp;
    fft_output->length = X.length;
    memcpy(fft_output->data, X.data, fft_output->length*sizeof(complex_s32_t));

    // Normalise to log max precision output
    left_shift_t ls = bfp_complex_s32_headroom(fft_output); // TODO Why is this not the same as fft_output->hr?
    bfp_complex_s32_shl(fft_output, fft_output, ls);
    fft_output->exp -= ls;
    
    // Calculate squared magnitude spectrum
    int32_t DWORD_ALIGNED squared_mag_data[VNR_FD_FRAME_LENGTH];
    bfp_s32_t squared_mag;
    bfp_s32_init(&squared_mag, squared_mag_data, 0, X.length, 0);
    bfp_complex_s32_squared_mag(&squared_mag, &X);
    
    //for debug
    squared_mag_out->hr = squared_mag.hr;
    squared_mag_out->exp = squared_mag.exp;
    squared_mag_out->length = squared_mag.length;
    memcpy(squared_mag_out->data, squared_mag.data, squared_mag_out->length*sizeof(int32_t));
    // Normalise to log max precision output
    ls = bfp_s32_headroom(squared_mag_out); 
    bfp_s32_shl(squared_mag_out, squared_mag_out, ls);
    squared_mag_out->exp -= ls;
    
    // MEL
    float_s32_t mel_output[AUDIO_FEATURES_NUM_MELS];
    vnr_mel_compute(mel_output, &X);

    fixed_s32_t log2_mel[AUDIO_FEATURES_NUM_MELS];
    vnr_log2(log2_mel, mel_output, AUDIO_FEATURES_NUM_MELS);
}
