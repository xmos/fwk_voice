#ifndef ADEC_PRIV_H
#define ADEC_PRIV_H

void init_pk_ave_ratio_history(adec_state_t *adec_state);
void reset_stuff_on_AEC_mode_start(adec_state_t *adec_state, unsigned set_toggle);
void set_delay_params_from_signed_delay(int32_t measured_delay, int32_t *mic_delay_samples, int32_t *requested_delay_debug);
fixed_s32_t float_to_frac_bits(float_s32_t value);
void push_peak_power_into_history(adec_state_t *adec_state, float_s32_t peak_phase_power);
void get_decimated_peak_power_history(float_s32_t peak_power_decimated[ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE], adec_state_t *adec_state);
float_s32_t linear_regression_get_slope(float_s32_t pk_energies[], unsigned n, float_s32_t scale_factor);
fixed_s32_t calculate_aec_goodness_metric(adec_state_t *state, fixed_s32_t log2erle_q24, float_s32_t peak_power_slope, fixed_s32_t agm_q24);

#endif
