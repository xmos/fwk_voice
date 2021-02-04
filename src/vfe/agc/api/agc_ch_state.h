// Copyright (c) 2017-2020, XMOS Ltd, All rights reserved
#ifndef _AGC_STATE_H_
#define _AGC_STATE_H_

#include <stdint.h>
#include "voice_toolbox.h"
#include "agc_conf.h"

#define AGC_ALPHA_SLOW_RISE VTB_UQ0_32(0.8869)
#define AGC_ALPHA_SLOW_FALL VTB_UQ0_32(0.9646)
#define AGC_ALPHA_FAST_RISE VTB_UQ0_32(0.3814)
#define AGC_ALPHA_FAST_FALL VTB_UQ0_32(0.8869)
#define AGC_ALPHA_PEAK_RISE VTB_UQ0_32(0.5480)
#define AGC_ALPHA_PEAK_FALL VTB_UQ0_32(0.9646)

#define AGC_LC_N_FRAME_NEAR (34)
#define AGC_LC_N_FRAME_FAR (17)
    
#define AGC_LC_EST_ALPHA_INC VTB_UQ0_32(0)
#define AGC_LC_EST_ALPHA_DEC VTB_UQ0_32(0.6973)
#define AGC_LC_BG_POWER_EST_ALPHA_DEC VTB_UQ0_32(0.5480)

#define AGC_LC_NEAR_POWER_EST VTB_UQ0_32(0.00001)
#define AGC_LC_BG_POWER_EST_INIT VTB_UQ0_32(0.01)
#define AGC_LC_FAR_BG_POWER_EST_INIT VTB_UQ0_32(0.01)

#define AGC_LC_CORR_THRESHOLD VTB_UQ0_32(0.9)


/**
 * Structure to hold AGC state, only used internally.
 */
typedef struct {
    int adapt;
    vtb_u32_float_t gain;
    vtb_u32_float_t max_gain;
    vtb_u32_float_t upper_threshold;
    vtb_u32_float_t lower_threshold;
    vtb_u32_float_t x_slow;
    vtb_u32_float_t x_fast;
    vtb_u32_float_t x_peak;
    vtb_u32_float_t gain_inc;
    vtb_u32_float_t gain_dec;
    int lc_enabled;
    int lc_t_far;
    int lc_t_near;
    vtb_u32_float_t lc_near_power_est;
    vtb_u32_float_t lc_far_power_est;
    vtb_u32_float_t lc_bg_power_est;
    vtb_u32_float_t lc_gain;
    vtb_u32_float_t lc_far_bg_power_est;
    vtb_uq0_32_t lc_corr_factor;
} agc_ch_state_t;


#endif // _AGC_STATE_H_
