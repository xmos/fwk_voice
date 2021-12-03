// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <xs3_math.h>
#include <bfp_math.h>
#include <meta_gen.h>

#define EXP  -31

#define DSP_FLOAT_ZERO_EXP (INT_MIN/2)
#define DSP_FLOAT_S32_ZERO {0, DSP_FLOAT_ZERO_EXP}
#define AP_FRAME_ADVANCE (240)
#define AEC_PROC_FRAME_LENGTH (512)

float_s32_t get_td_frame_energy(int32_t * x, unsigned frame_length){
    bfp_s32_t ch_arr;
    float_s64_t en_s64;
    float_s32_t en_s32;
    bfp_s32_init( &ch_arr, x, EXP, frame_length, 1);
    en_s64 = bfp_s32_energy(&ch_arr);
    en_s32 = float_s64_to_float_s32(en_s64);
    return en_s32;
}

float_s32_t get_max_ref_energy(int32_t * x[], unsigned frame_length, int num_chan){
    float_s32_t current, max;

    max = get_td_frame_energy(x[0], frame_length);
    for (unsigned s = 1; s < num_chan; s++){
        current = get_td_frame_energy(x[s], frame_length);
        if(float_s32_gt(current, max)){max = current;}
    }
    return max;
}

float_s32_t get_aec_corr_factor( const int32_t * mic_data, const bfp_s32_t * y_hat){
    const int FRAME_WINDOW = AEC_PROC_FRAME_LENGTH % AP_FRAME_ADVANCE;
    float_s64_t aec_corr_numerator = DSP_FLOAT_S32_ZERO, aec_corr_denom = DSP_FLOAT_S32_ZERO;
    int32_t scratch[AP_FRAME_ADVANCE - FRAME_WINDOW], scratch2[AP_FRAME_ADVANCE - FRAME_WINDOW];
    float_s32_t t;

    bfp_s32_t mic_frame, mic_frame_abs, y_hat_abs, y_hat_short;

    bfp_s32_init(&mic_frame,mic_data,EXP, AP_FRAME_ADVANCE - FRAME_WINDOW,1);
    bfp_s32_init(&mic_frame_abs, scratch, EXP, AP_FRAME_ADVANCE - FRAME_WINDOW, 1);
    bfp_s32_init(&y_hat_abs, scratch2, EXP, AP_FRAME_ADVANCE - FRAME_WINDOW, 1);
    bfp_s32_init(&y_hat_short, y_hat->data, EXP, AP_FRAME_ADVANCE - FRAME_WINDOW, 1);

    aec_corr_numerator = bfp_s32_dot(&mic_frame, &y_hat_short); 

    bfp_s32_abs(&mic_frame_abs, &mic_frame);

    bfp_s32_abs(&y_hat_abs, &y_hat_short);

    aec_corr_denom = bfp_s32_dot(&mic_frame_abs, &y_hat_abs);

    t = float_s32_div(float_s32_abs(float_s64_to_float_s32(aec_corr_numerator)), float_s64_to_float_s32(aec_corr_denom));

    return t;
}
