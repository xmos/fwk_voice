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
    input_state->vnr_features_config.input_scale_inv = double_to_float_s32(1/0.09976799786090851); //from interpreter_tflite.get_input_details()[0] call in python 
    input_state->vnr_features_config.input_zero_point = double_to_float_s32(127);

    input_state->vnr_features_config.output_scale = double_to_float_s32(0.00390625); //from interpreter_tflite.get_output_details()[0] call in python 
    input_state->vnr_features_config.output_zero_point = double_to_float_s32(-128);
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

void vnr_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice)
{
    for(unsigned index=0; index<VNR_PATCH_WIDTH - 1; index++) {
        memcpy(feature_buffers[index], feature_buffers[index+1], VNR_MEL_FILTERS*sizeof(int32_t));
    }
    memcpy(feature_buffers[VNR_PATCH_WIDTH - 1], new_slice, VNR_MEL_FILTERS*sizeof(int32_t));
}

void vnr_normalise_patch(bfp_s32_t *normalised_patch, const fixed_s32_t (*feature_patch)[VNR_MEL_FILTERS])
{
    //norm_patch = feature_patch - np.max(feature_patch)   
    bfp_s32_t feature_patch_bfp;
    bfp_s32_init(&feature_patch_bfp, (int32_t*)&feature_patch[0][0], VNR_LOG2_OUTPUT_EXP, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    float_s32_t max = bfp_s32_max(&feature_patch_bfp);
    float_s32_t zero = {0, 0};
    float_s32_t neg_max = float_s32_sub(zero, max);
    bfp_s32_add_scalar(normalised_patch, &feature_patch_bfp, neg_max);
    /*printf("framenum %d\n",framenum);
    printf("max = %.6f\n",ldexp(max.mant, max.exp));
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/

}
#include <xcore/hwtimer.h>
void vnr_quantise_patch(int8_t *quantised_patch, bfp_s32_t *normalised_patch, const vnr_features_config_t *features_config) {
    uint64_t start = (uint64_t)get_reference_time();
    // this_patch = this_patch / input_scale + input_zero_point        
    bfp_s32_scale(normalised_patch, normalised_patch, features_config->input_scale_inv);
    bfp_s32_add_scalar(normalised_patch, normalised_patch, features_config->input_zero_point);
    // this_patch = np.round(this_patch)
    bfp_s32_use_exponent(normalised_patch, -24);

    /*printf("framenum %d\n",framenum);
    printf("Before rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/
    //Test a few values
    /*normalised_patch->data[0] = (int)((2.3278371)*(1<<24));
    normalised_patch->data[1] = (int)((-2.3278371)*(1<<24));
    normalised_patch->data[2] = (int)((2.5809)*(1<<24));
    normalised_patch->data[3] = (int)((-2.5809)*(1<<24));
    normalised_patch->data[4] = (int)((2.50000)*(1<<24));
    normalised_patch->data[5] = (int)((-2.50000)*(1<<24));
    normalised_patch->data[6] = (int)((4.5)*(1<<24));
    normalised_patch->data[7] = (int)((-4.5)*(1<<24));
    normalised_patch->data[8] = (int)((5.5)*(1<<24));
    normalised_patch->data[9] = (int)((-5.5)*(1<<24));
    normalised_patch->data[10] = (int)((5.51)*(1<<24));
    normalised_patch->data[11] = (int)((-5.51)*(1<<24));*/
    float_s32_t round_lsb = {(1<<23), -24};
    bfp_s32_add_scalar(normalised_patch, normalised_patch, round_lsb);
    bfp_s32_use_exponent(normalised_patch, -24);
    left_shift_t ls = -24;
    bfp_s32_shl(normalised_patch, normalised_patch, ls);
    normalised_patch->exp -= -24; 
    /*printf("In rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp));
    }*/
    
    for(int i=0; i<96; i++) {
        quantised_patch[i] = (int8_t)normalised_patch->data[i]; 
    }

    uint64_t end = (uint64_t)get_reference_time();

    /*printf("quant_cycles = %ld\n", (int32_t)(end-start));
    printf("After rounding\n");
    for(int i=0; i<VNR_MEL_FILTERS*VNR_PATCH_WIDTH; i++) {
        printf("patch[%d] = %.6f, final_val = %d\n",i, ldexp(normalised_patch->data[i], normalised_patch->exp), quantised_patch[i]);
    }*/
    framenum++;
}

void vnr_dequantise_output(float_s32_t *dequant_output, const int8_t* quant_output, const vnr_features_config_t *features_config) {
    dequant_output->mant = ((int32_t)*quant_output) << 24; //8.24
    dequant_output->exp = -24;
    *dequant_output = float_s32_sub(*dequant_output, features_config->output_zero_point);
    *dequant_output = float_s32_mul(*dequant_output, features_config->output_scale);
}

void vnr_extract_features(int8_t* quantised_patch, vnr_input_state_t *input_state, const int32_t *new_x_frame)
{
    int32_t DWORD_ALIGNED x_data[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
    vnr_form_input_frame(input_state, x_data, new_x_frame);
    
    bfp_complex_s32_t X;
    vnr_forward_fft(&X, x_data);

    // MEL
    float_s32_t mel_output[AUDIO_FEATURES_NUM_MELS];
    vnr_mel_compute(mel_output, &X);

    fixed_s32_t log2_mel[AUDIO_FEATURES_NUM_MELS];
    vnr_log2(log2_mel, mel_output, AUDIO_FEATURES_NUM_MELS);

    vnr_add_new_slice(input_state->feature_buffers, log2_mel);
#if (VNR_FD_FRAME_LENGTH < (VNR_MEL_FILTERS*VNR_PATCH_WIDTH))
    #error ERROR squared_mag_data memory not enough for reuse as normalised_patch
#endif
    // Reuse x_data memory for normalised_patch
    bfp_s32_t normalised_patch;
    bfp_s32_init(&normalised_patch, x_data, 0, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    vnr_normalise_patch(&normalised_patch, input_state->feature_buffers);

    vnr_quantise_patch(quantised_patch, &normalised_patch, &input_state->vnr_features_config); 
}
