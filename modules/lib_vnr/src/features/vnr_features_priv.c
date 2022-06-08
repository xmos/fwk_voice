
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "vnr_features_priv.h"
#include "mel_filter_512_24_compact.h"

void vnr_priv_forward_fft(bfp_complex_s32_t *X, int32_t *x_data) {
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
    temp->hr = bfp_complex_s32_headroom(temp); // TODO Workaround till https://github.com/xmos/lib_xs3_math/issues/96 is fixed
    bfp_fft_unpack_mono(temp);
    memcpy(X, temp, sizeof(bfp_complex_s32_t));
}

void vnr_priv_make_slice(fixed_s32_t *new_slice, const bfp_complex_s32_t *X) {
    // MEL
    float_s32_t mel_output[VNR_MEL_FILTERS];
    
    vnr_priv_mel_compute(mel_output, X);

    vnr_priv_log2(new_slice, mel_output, VNR_MEL_FILTERS); //Calculate new_slice in state->scratch_data
}

void vnr_priv_add_new_slice(fixed_s32_t (*feature_buffers)[VNR_MEL_FILTERS], const fixed_s32_t *new_slice)
{
    for(unsigned index=0; index<VNR_PATCH_WIDTH - 1; index++) {
        memcpy(feature_buffers[index], feature_buffers[index+1], VNR_MEL_FILTERS*sizeof(int32_t));
    }
    memcpy(feature_buffers[VNR_PATCH_WIDTH - 1], new_slice, VNR_MEL_FILTERS*sizeof(int32_t));
}

void vnr_priv_normalise_patch(bfp_s32_t *normalised_patch, int32_t *normalised_patch_data, const vnr_feature_state_t *feature_state)
{
#if (VNR_FD_FRAME_LENGTH < (VNR_MEL_FILTERS*VNR_PATCH_WIDTH))
    #error ERROR squared_mag_data memory not enough for reuse as normalised_patch
#endif
    bfp_s32_init(normalised_patch, normalised_patch_data, 0, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    //norm_patch = feature_patch - np.max(feature_patch)   
    bfp_s32_t feature_patch_bfp;
    bfp_s32_init(&feature_patch_bfp, (int32_t*)&feature_state->feature_buffers[0][0], VNR_LOG2_OUTPUT_EXP, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    float_s32_t max = bfp_s32_max(&feature_patch_bfp);
    float_s32_t zero = {0, 0};
    float_s32_t neg_max = float_s32_sub(zero, max);
    bfp_s32_add_scalar(normalised_patch, &feature_patch_bfp, neg_max);
}
void vnr_priv_mel_compute(float_s32_t *filter_output, const bfp_complex_s32_t *X) {
#if VNR_MEL_FILTERS != AUDIO_FEATURES_NUM_MELS
    #error VNR_MEL_FILTERS not the same as AUDIO_FEATURES_NUM_MELS
#endif
    // Calculate squared magnitude spectrum
    int32_t DWORD_ALIGNED squared_mag_data[VNR_FD_FRAME_LENGTH];
    bfp_s32_t squared_mag;
    bfp_s32_init(&squared_mag, squared_mag_data, 0, X->length, 0);
    bfp_complex_s32_squared_mag(&squared_mag, X);
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
        bfp_s32_init(&spect_subset, &squared_mag.data[filter_start], squared_mag.exp, filter_length, 1);

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
    bfp_s32_init(&odd_band_filters, odd_band_filters_data, 0, AUDIO_FEATURES_NUM_BINS, 0);
    bfp_s32_set(&odd_band_filters, AUDIO_FEATURES_MEL_MAX, filter_exp);
    odd_band_filters.hr = HR_S32(AUDIO_FEATURES_MEL_MAX);
    bfp_s32_sub(&odd_band_filters, &odd_band_filters, &filter);

    for(unsigned i=0; i<num_odd_filters; i++) {
        unsigned filter_start = mel_filter_512_24_compact_start_bins[(2*i) + 1];
        unsigned filter_length = mel_filter_512_24_compact_start_bins[(2*(i+1)) + 1] - filter_start;
        //Create the bfp for spectrum subset
        bfp_s32_t spect_subset;
        bfp_s32_init(&spect_subset, &squared_mag.data[filter_start], squared_mag.exp, filter_length, 1);

        //Create BFP for the filter
        bfp_s32_t filter_subset;
        bfp_s32_init(&filter_subset, &odd_band_filters.data[filter_start], odd_band_filters.exp, filter_length, 0);
        filter_subset.hr = odd_band_filters.hr;

        filter_output[(2*i)+1] = float_s64_to_float_s32(bfp_s32_dot(&spect_subset, &filter_subset));
    }
}

void vnr_priv_log2(fixed_s32_t *output_q24, const float_s32_t *input, unsigned length) {
    for(unsigned i=0; i<length; i++) {
        output_q24[i] = vnr_priv_float_s32_to_fixed_q24_log2(input[i]);
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


// Lookup table entries k=0,1,...,32:
//lookup[k] = 256log2(1 + k/32)
static int lookup_small_log2_linear_new(uint32_t x) {
    int mask_bits = 26;
    int mask = (1 << mask_bits) - 1;
    int y = (x >> mask_bits) - 32;
    int y1 = y + 1;
    int v0 = lookup[y], v1 = lookup[y1];
    int f1 = x & mask;
    int f0 = mask + 1 - f1;
    return (v0 * (uint64_t) f0 + v1 * (uint64_t) f1) >> (mask_bits - (MEL_PRECISION - LOOKUP_PRECISION));
}

fixed_s32_t vnr_priv_float_s32_to_fixed_q24_log2(float_s32_t x) {
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
