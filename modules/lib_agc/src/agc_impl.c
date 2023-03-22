// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <limits.h>
#include "agc_defines.h"
#include "xmath/xmath.h"
#include <agc_api.h>

void agc_init(agc_state_t *agc, agc_config_t *config)
{
    agc->config = *config;

    agc->x_slow = f32_to_float_s32(0);
    agc->x_fast = f32_to_float_s32(0);
    agc->x_peak = f32_to_float_s32(0);

    agc->lc_t_far = 0;
    agc->lc_t_near = 0;

    agc->lc_near_power_est = f32_to_float_s32(0.00001F);
    agc->lc_far_power_est = f32_to_float_s32(0.01F);
    agc->lc_near_bg_power_est = f32_to_float_s32(0.01F);
    agc->lc_gain = f32_to_float_s32(1);
    agc->lc_far_bg_power_est = f32_to_float_s32(0.01F);
    agc->lc_corr_val = f32_to_float_s32(0);
}

// Returns the mantissa for the input float shifted to an exponent of parameter exp
static int32_t use_exp_float(float_s32_t fl, exponent_t exp)
{
    exponent_t exp_diff = fl.exp - exp;

    if (exp_diff > 0) {
        return fl.mant << exp_diff;
    } else if (exp_diff < 0) {
        return fl.mant >> -exp_diff;
    }

    return fl.mant;
}

// Returns the soft-clipped mantissa in terms of the original exponent
static int32_t apply_soft_clipping(int32_t mant, exponent_t exp)
{
    float_s32_t sample = {mant, exp};
    float_s32_t sample_abs = float_s32_abs(sample);

    if (float_s32_gt(AGC_SOFT_CLIPPING_THRESH, sample_abs)) {
        return mant;
    }

    // Division by zero is not possible after the absolute value test against AGC_LC_LIMIT_POINT
    float_s32_t sample_limit = float_s32_div(AGC_SOFT_CLIPPING_NUMERATOR, sample_abs);
    sample_limit = float_s32_sub(FLOAT_S32_ONE, sample_limit);

    if (float_s32_gt(FLOAT_S32_ZERO, sample)) {
        sample_limit = float_s32_sub(FLOAT_S32_ZERO, sample_limit);
    }

    return use_exp_float(sample_limit, exp);
}

void agc_process_frame(agc_state_t *agc,
                       int32_t output[AGC_FRAME_ADVANCE],
                       const int32_t input[AGC_FRAME_ADVANCE],
                       agc_meta_data_t *meta_data)
{
    int vnr_flag = meta_data->vnr_flag;

    if (agc->config.adapt_on_vnr == 0) {
        vnr_flag = 1;
    }

    bfp_s32_t input_bfp;
    bfp_s32_init(&input_bfp, (int32_t *)input, FRAME_EXP, AGC_FRAME_ADVANCE, 1);

    bfp_s32_t output_bfp;
    bfp_s32_init(&output_bfp, (int32_t *)output, FRAME_EXP, AGC_FRAME_ADVANCE, 0);

    if (agc->config.adapt) {
        // Get max absolute sample value by comparing the absolute values of the min and max.
        // An alternative approach is to form a new vector with the absolute values and then find
        // the max value, which took 48 fewer cycles but required an extra 760 bytes of memory.
        float_s32_t max_sample = float_s32_abs(bfp_s32_max(&input_bfp));
        float_s32_t min_sample = float_s32_abs(bfp_s32_min(&input_bfp));
        float_s32_t max_abs_value;

        if (float_s32_gte(max_sample, min_sample)) {
            max_abs_value = max_sample;
        } else {
            max_abs_value = min_sample;
        }

        unsigned rising = float_s32_gte(max_abs_value, agc->x_slow);
        if (rising) {
            agc->x_slow = float_s32_ema(agc->x_slow, max_abs_value, AGC_ALPHA_SLOW_RISE);
            agc->x_fast = float_s32_ema(agc->x_fast, max_abs_value, AGC_ALPHA_FAST_RISE);
        } else {
            agc->x_slow = float_s32_ema(agc->x_slow, max_abs_value, AGC_ALPHA_SLOW_FALL);
            agc->x_fast = float_s32_ema(agc->x_fast, max_abs_value, AGC_ALPHA_FAST_FALL);
        }

        float_s32_t gained_max_abs_value = float_s32_mul(max_abs_value, agc->config.gain);
        unsigned exceed_threshold = float_s32_gte(gained_max_abs_value, agc->config.upper_threshold);

        if (exceed_threshold || vnr_flag) {
            unsigned peak_rising = float_s32_gte(agc->x_fast, agc->x_peak);
            if (peak_rising) {
                agc->x_peak = float_s32_ema(agc->x_peak, agc->x_fast, AGC_ALPHA_PEAK_RISE);
            } else {
                agc->x_peak = float_s32_ema(agc->x_peak, agc->x_fast, AGC_ALPHA_PEAK_FALL);
            }

            float_s32_t gained_pk = float_s32_mul(agc->x_peak, agc->config.gain);
            unsigned near_only = (agc->lc_t_near != 0) && (agc->lc_t_far == 0);
            if (float_s32_gte(gained_pk, agc->config.upper_threshold)) {
                agc->config.gain = float_s32_mul(agc->config.gain_dec, agc->config.gain);
            } else if (float_s32_gte(agc->config.lower_threshold, gained_pk) &&
                       (agc->config.lc_enabled == 0 || near_only != 0)) {
                agc->config.gain = float_s32_mul(agc->config.gain_inc, agc->config.gain);
            }

            if (float_s32_gte(agc->config.gain, agc->config.max_gain)) {
                agc->config.gain = agc->config.max_gain;
            }

            if (float_s32_gte(agc->config.min_gain, agc->config.gain)) {
                agc->config.gain = agc->config.min_gain;
            }
        }
    }

    float_s32_t frame_power = float_s64_to_float_s32(bfp_s32_energy(&input_bfp));
    bfp_s32_scale(&output_bfp, &input_bfp, agc->config.gain);

    // Update loss control state

    if (float_s32_gte(agc->lc_far_power_est, meta_data->aec_ref_power)) {
        agc->lc_far_power_est = float_s32_ema(agc->lc_far_power_est, meta_data->aec_ref_power, AGC_ALPHA_LC_EST_DEC);
    } else {
        agc->lc_far_power_est = float_s32_ema(agc->lc_far_power_est, meta_data->aec_ref_power, AGC_ALPHA_LC_EST_INC);
    }

    float_s32_t far_bg_power_est = float_s32_mul(agc->config.lc_bg_power_gamma, agc->lc_far_bg_power_est);
    if (float_s32_gte(far_bg_power_est, agc->lc_far_power_est)) {
        agc->lc_far_bg_power_est = agc->lc_far_power_est;
    } else {
        agc->lc_far_bg_power_est = far_bg_power_est;
    }

    if (float_s32_gte(AGC_LC_FAR_BG_POWER_EST_MIN, agc->lc_far_bg_power_est)) {
        agc->lc_far_bg_power_est = AGC_LC_FAR_BG_POWER_EST_MIN;
    }

    if (float_s32_gte(agc->lc_near_power_est, frame_power)) {
        agc->lc_near_power_est = float_s32_ema(agc->lc_near_power_est, frame_power, AGC_ALPHA_LC_EST_DEC);
    } else {
        agc->lc_near_power_est = float_s32_ema(agc->lc_near_power_est, frame_power, AGC_ALPHA_LC_EST_INC);
    }

    if (float_s32_gt(agc->lc_near_bg_power_est, agc->lc_near_power_est)) {
        agc->lc_near_bg_power_est = float_s32_ema(agc->lc_near_bg_power_est, agc->lc_near_power_est, AGC_ALPHA_LC_BG_POWER_EST_DEC);
    } else {
        agc->lc_near_bg_power_est = float_s32_mul(agc->config.lc_bg_power_gamma, agc->lc_near_bg_power_est);
    }

    if (agc->config.lc_enabled) {
        if (float_s32_gt(meta_data->aec_corr_factor, agc->lc_corr_val)) {
            agc->lc_corr_val = meta_data->aec_corr_factor;
        } else {
            agc->lc_corr_val = float_s32_ema(agc->lc_corr_val, meta_data->aec_corr_factor, AGC_ALPHA_LC_CORR);
        }

        if (float_s32_gt(agc->lc_far_power_est, float_s32_mul(agc->config.lc_far_delta, agc->lc_far_bg_power_est))) {
            agc->lc_t_far = agc->config.lc_n_frame_far;
        } else {
            if (agc->lc_t_far > 0) {
                --agc->lc_t_far;
            }
        }

        float_s32_t delta = (agc->lc_t_far > 0) ? agc->config.lc_near_delta_far_active : agc->config.lc_near_delta;

        if (float_s32_gt(agc->lc_near_power_est, float_s32_mul(delta, agc->lc_near_bg_power_est))) {
            if (agc->lc_t_far == 0 || (agc->lc_t_far > 0 &&
                                       float_s32_gt(agc->config.lc_corr_threshold, agc->lc_corr_val))) {
                // Near-end speech only or double talk
                agc->lc_t_near = agc->config.lc_n_frame_near;
            } else {
                // Far-end speech only
                // Do nothing
            }
        } else {
            // Silence
            if (agc->lc_t_near > 0) {
                --agc->lc_t_near;
            }
        }

        // Adapt loss control gain
        float_s32_t lc_target_gain;
        if (agc->lc_t_far <= 0 && agc->lc_t_near > 0) {
            // Near-end only
            lc_target_gain = agc->config.lc_gain_max;
        } else if (agc->lc_t_far <= 0 && agc->lc_t_near <= 0) {
            // Silence
            lc_target_gain = agc->config.lc_gain_silence;
        } else if (agc->lc_t_far > 0 && agc->lc_t_near <= 0) {
            // Far-end only
            lc_target_gain = agc->config.lc_gain_min;
        } else {
            // Double talk
            lc_target_gain = agc->config.lc_gain_double_talk;
        }

        // When changing from one value of lc_target_gain to a different one, the change
        // is applied gradually, sample-by-sample in the frame, using lc_gamma_inc/dec.
        // The lc_scale array is initially set to the target value and then overwritten
        // from the beginning as required to transition from the previous lc_gain value.
        // This will create a BFP array representing the gradual scale changes which
        // can be applied by multiplying element-wise using the VPU.
        int32_t lc_scale[AGC_FRAME_ADVANCE];
        bfp_s32_t lc_scale_bfp;
        bfp_s32_init(&lc_scale_bfp, lc_scale, lc_target_gain.exp, AGC_FRAME_ADVANCE, 0);
        bfp_s32_set(&lc_scale_bfp, lc_target_gain.mant, lc_target_gain.exp);
        // Add some headroom to avoid changing the exponent when gradually transitioning from
        // previous lc_gain to lc_target_gain. Anyway, 32 bits of precision is unnecessary.
        bfp_s32_shl(&lc_scale_bfp, &lc_scale_bfp, -8);
        lc_scale_bfp.exp += 8;

        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            if (float_s32_gt(agc->lc_gain, lc_target_gain)) {
                agc->lc_gain = float_s32_mul(agc->lc_gain, agc->config.lc_gamma_dec);
                if (float_s32_gt(lc_target_gain, agc->lc_gain)) {
                    agc->lc_gain = lc_target_gain;
                }
                lc_scale[idx] = use_exp_float(agc->lc_gain, lc_target_gain.exp);
            } else if (float_s32_gt(lc_target_gain, agc->lc_gain)) {
                agc->lc_gain = float_s32_mul(agc->lc_gain, agc->config.lc_gamma_inc);
                if (float_s32_gt(agc->lc_gain, lc_target_gain)) {
                    agc->lc_gain = lc_target_gain;
                }
                lc_scale[idx] = use_exp_float(agc->lc_gain, lc_target_gain.exp);
            } else {
                break;
            }
        }

        bfp_s32_mul(&output_bfp, &output_bfp, &lc_scale_bfp);
    }

    if (agc->config.soft_clipping) {
        for (unsigned idx = 0; idx < AGC_FRAME_ADVANCE; ++idx) {
            output[idx] = apply_soft_clipping(output[idx], output_bfp.exp);
        }
    }

    bfp_s32_use_exponent(&output_bfp, FRAME_EXP);
}
