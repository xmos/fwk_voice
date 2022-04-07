// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include "vnr_features_api.h"
#include "mel_filter_512_24_compact.h"

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
    bfp_fft_unpack_mono(temp);
    memcpy(X, temp, sizeof(bfp_complex_s32_t));
}

void vnr_mel_compute(bfp_s32_t *squared_mag_spect, float_s64_t *filter_output) {
    unsigned num_bands = AUDIO_FEATURES_NUM_MELS + 1; //Extra band for the 2nd half of the last filter  
    unsigned num_even_filters = num_bands / 2;
    unsigned num_odd_filters = AUDIO_FEATURES_NUM_MELS - num_even_filters;
    int filter_exp = -(31-AUDIO_FEATURES_MEL_HEADROOM_BITS);
    for(unsigned i=0; i<num_even_filters; i++) {
        unsigned filter_start = mel_filter_512_24_compact_start_bins[2*i];
        unsigned filter_length = mel_filter_512_24_compact_start_bins[2*(i+1)] - filter_start;
        //Create the bfp for spectrum subset
        bfp_s32_t spect_subset;
        bfp_s32_init(&spect_subset, &squared_mag_spect->data[filter_start], squared_mag_spect->exp, filter_length, 0);
        spect_subset.hr = squared_mag_spect->hr;

        //Create BFP for the filter
        bfp_s32_t filter_subset;
        bfp_s32_init(&filter_subset, &mel_filter_512_24_compact_q31[filter_start], filter_exp, filter_length, 0);

        filter_output[2*i] = bfp_s32_dot(&spect_subset, &filter_subset);
    }

    int32_t odd_band_filters_data[AUDIO_FEATURES_NUM_BINS];
    bfp_s32_t odd_band_filters;
    bfp_s32_init(&odd_band_filters, odd_band_filters_data, 0, AUDIO_FEATURES_NUM_BINS, HR_S32(AUDIO_FEATURES_MEL_MAX));
    bfp_s32_set(&odd_band_filters, AUDIO_FEATURES_MEL_MAX, filter_exp);

    for(unsigned i=0; i<num_odd_filters; i++) {
        unsigned filter_start = mel_filter_512_24_compact_start_bins[(2*i) + 1];
        unsigned filter_length = mel_filter_512_24_compact_start_bins[(2*(i+1)) + 1] - filter_start;
        //Create the bfp for spectrum subset
        bfp_s32_t spect_subset;
        bfp_s32_init(&spect_subset, &squared_mag_spect->data[filter_start], squared_mag_spect->exp, filter_length, 0);
        spect_subset.hr = squared_mag_spect->hr;

        //Create BFP for the filter
        bfp_s32_t filter_subset;
        bfp_s32_init(&filter_subset, &mel_filter_512_24_compact_q31[filter_start], filter_exp, filter_length, 0);

        filter_output[(2*i)+1] = bfp_s32_dot(&spect_subset, &filter_subset);
    }
}

static int framenum=0;
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
    float_s64_t mel_output[AUDIO_FEATURES_NUM_MELS];
    vnr_mel_compute(&squared_mag, mel_output);

    framenum++;
}
