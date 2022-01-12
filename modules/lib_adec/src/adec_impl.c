// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "adec_api.h"
#include <string.h>
#include <math.h>
#include <limits.h>

void init_pk_ave_ratio_history(adec_state_t *adec_state){
  const float_s32_t float_s32_thousand = {2097160000,-21}; //A large number we never normally see, so it will be clear when we get an actual
  for (int i = 0; i < ADEC_PEAK_TO_RAGE_HISTORY_DEPTH; i++){
    adec_state->peak_to_average_ratio_history[i] = float_s32_thousand;
  }
}

void reset_stuff_on_AEC_mode_start(adec_state_t *adec_state, unsigned set_toggle){
  //printf("In reset_stuff_on_AEC_mode_start()\n");
  adec_state->agm_q24 = ADEC_AGM_HALF;

  init_pk_ave_ratio_history(adec_state);

  memset(adec_state->peak_power_history, 0, ADEC_PEAK_LINREG_HISTORY_SIZE * sizeof(adec_state->peak_power_history[0]));
  adec_state->peak_power_history_idx = 0;
  adec_state->peak_power_history_valid = 0;

  adec_state->peak_to_average_ratio_valid_flag = 0;
  adec_state->max_peak_to_average_ratio_since_reset = double_to_float_s32(1.0);

  adec_state->gated_milliseconds_since_mode_change = 0;

  if (set_toggle) {
    adec_state->sf_copy_flag = 0;
    adec_state->shadow_flag_counter = 0;
  }
}

/* Delay samples refer to mic samples delay.
 * +ve delay value. Mic is early so delay mic. This means the echo seen in mic input is not seen on reference input yet.
 * This is the really bad case.
 * -ve delay value means mic is late so advance mic. Obviously, can't advance in time, so we delay reference instead.
 * Mic late means the echo seen in mic input is present in one of the (ph>0) phases of X_fifo. So the filter peak is not
 * at H_hat[0] and we're wasting some of the tail length.
 */
void adec_init(adec_state_t *adec_state){
  adec_state->adec_config.bypass = 0;
  adec_state->adec_config.force_de_cycle_trigger = 0;
  adec_state->agm_q24 = ADEC_AGM_HALF;

  //Using bits log2(erle) with q7_28 gives us up to 10log(2^127) = 382dB ERLE measurement range.. 
  adec_state->erle_bad_bits_q24 = FLOAT_TO_Q24(ADEC_ERLE_BAD_BITS);
  adec_state->erle_good_bits_q24 = FLOAT_TO_Q24(ADEC_ERLE_GOOD_BITS);
  adec_state->peak_phase_energy_trend_gain_q24 = FLOAT_TO_Q24(ADEC_PEAK_PHASE_ENERGY_TREND_GAIN);
  adec_state->erle_bad_gain_q24 = FLOAT_TO_Q24(ADEC_ERLE_BAD_GAIN);

  adec_state->peak_to_average_ratio_valid_flag = 0;
  adec_state->max_peak_to_average_ratio_since_reset = double_to_float_s32(1.0);

  float_s32_t v = ADEC_PEAK_TO_AVERAGE_GOOD_AEC;
  adec_state->aec_peak_to_average_good_aec_threshold = v; 

  adec_state->mode = ADEC_NORMAL_AEC_MODE;
  adec_state->gated_milliseconds_since_mode_change = 0;
  adec_state->last_measured_delay = 0;

  for (int i = 0; i < ADEC_PEAK_LINREG_HISTORY_SIZE; i++){
    adec_state->peak_power_history[i] = double_to_float_s32(0.0);
  }
  adec_state->peak_power_history_idx = 0;
  adec_state->peak_power_history_valid = 0;

  init_pk_ave_ratio_history(adec_state);

  adec_state->sf_copy_flag = 0;
  adec_state->convergence_counter = 0;
  adec_state->shadow_flag_counter = 0;
  adec_state->measured_delay_samples_debug = 0;
}

//Work out what we should send to the delay register from a signed input. Accounts for the margin to ensure mics always after ref
void set_delay_params_from_signed_delay(int32_t measured_delay, int32_t *mic_delay_samples, int32_t *measured_delay_compensated_debug){
    //If we see a MIC delay (+ve measured delay), we want to delay Reference by LESS than this to leave some headroom
    //If we see a REF delay (-ve measured delay), we want to delay MIC by MORE than this to leave some headroom
    int32_t measured_delay_compensated = measured_delay - ADEC_DE_DELAY_HEADROOM_SAMPS;

    //Set the requested mic delay to -ve of the measured delay in order to compensate the effect of delay measured in the system
    *mic_delay_samples = -(measured_delay_compensated);
    //Make sure we don't exceed the delay buffer size

    if (*mic_delay_samples >= MAX_DELAY_SAMPLES){
        int32_t actual_delay = MAX_DELAY_SAMPLES - 1; //-1 because we cannot support the actual maximum delay in the buffer (it wraps to zero)
        printf("**Warning - too large a delay requested (%ld), setting to %ld\n", *mic_delay_samples, actual_delay);
        *mic_delay_samples = actual_delay;
    }
    else if(*mic_delay_samples <= -(MAX_DELAY_SAMPLES)) {
        int32_t actual_delay = -(MAX_DELAY_SAMPLES - 1); //-1 because we cannot support the actual maximum delay in the buffer (it wraps to zero)
        printf("**Warning - too large a delay requested (%ld), setting to %ld\n", *mic_delay_samples, actual_delay);
        *mic_delay_samples = actual_delay;
    }

    //Write to debug copy of var for logging purposes during simulations
    //We toggle the lower bit to ensure a set to same value counts as a change. 1 sample is inconsequential to the ADEC
    unsigned new_bottom_bit = (*measured_delay_compensated_debug & 0x1) ^ 0x1; 
    *measured_delay_compensated_debug = -(measured_delay_compensated);
    *measured_delay_compensated_debug = (*measured_delay_compensated_debug & 0xfffffffe) | new_bottom_bit;
}

//This takes about 68 cycles compared with dsp_math_log that takes 12650
//See unit test for const array generation and test. Accuracy is better than 0.06dB with this table & config
fixed_s32_t float_to_frac_bits(float_s32_t value){
  const LOG2_LOOKUP_TYPE log2_lookup[LOG2_LOOKUP_TABLE_SIZE] = {
    0x0, 0x0, 0x1, 0x2, 0x2, 0x3, 0x4, 0x4, 0x5, 0x6, 0x7, 0x7, 0x8, 0x9, 0x9, 0xa, 0xb, 0xb, 0xc, 0xd, 0xd, 0xe, 
    0xf, 0xf, 0x10, 0x11, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x15, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x1a, 0x1a, 
    0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1f, 0x1f, 0x20, 0x20, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x25, 0x25, 0x26, 
    0x26, 0x27, 0x28, 0x28, 0x29, 0x29, 0x2a, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x30, 0x30, 0x31, 
    0x31, 0x32, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x36, 0x36, 0x37, 0x37, 0x38, 0x38, 0x39, 0x39, 0x3a, 0x3a, 0x3b, 
    0x3b, 0x3c, 0x3c, 0x3d, 0x3d, 0x3e, 0x3e, 0x3f, 0x3f, 0x40, 0x40, 0x41, 0x42, 0x42, 0x43, 0x43, 0x44, 0x44, 0x45, 
    0x45, 0x46, 0x46, 0x46, 0x47, 0x47, 0x48, 0x48, 0x49, 0x49, 0x4a, 0x4a, 0x4b, 0x4b, 0x4c, 0x4c, 0x4d, 0x4d, 0x4e, 
    0x4e, 0x4f, 0x4f, 0x50, 0x50, 0x51, 0x51, 0x51, 0x52, 0x52, 0x53, 0x53, 0x54, 0x54, 0x55, 0x55, 0x56, 0x56, 0x56, 
    0x57, 0x57, 0x58, 0x58, 0x59, 0x59, 0x5a, 0x5a, 0x5a, 0x5b, 0x5b, 0x5c, 0x5c, 0x5d, 0x5d, 0x5e, 0x5e, 0x5e, 0x5f, 
    0x5f, 0x60, 0x60, 0x61, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x66, 0x67, 
    0x67, 0x68, 0x68, 0x68, 0x69, 0x69, 0x6a, 0x6a, 0x6b, 0x6b, 0x6b, 0x6c, 0x6c, 0x6d, 0x6d, 0x6d, 0x6e, 0x6e, 0x6f, 
    0x6f, 0x6f, 0x70, 0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x72, 0x73, 0x73, 0x74, 0x74, 0x74, 0x75, 0x75, 0x75, 0x76, 
    0x76, 0x77, 0x77, 0x77, 0x78, 0x78, 0x79, 0x79, 0x79, 0x7a, 0x7a, 0x7a, 0x7b, 0x7b, 0x7b, 0x7c, 0x7c, 0x7d, 0x7d, 
    0x7d, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f};
  
  //Convert from (known to be positive) signed to unsigned
  int headroom = HR_S32(value.mant);
  //since value is known to be positive, increase headroom by 1, since this function works on a normalised unsigned value
  headroom = headroom + 1;
  uint32_t value_mant;
  int32_t value_exp;
  value_mant = (uint32_t)value.mant << headroom;
  value_exp = value.exp - headroom;

  //Handle case for zero mantissa which is possible with float_s32_t
  if (value_mant == 0) return INT_MIN;

  int32_t number_of_bits = 31 + value_exp;

  const uint32_t frac_bits = (uint32_t)value_mant - (uint32_t)0x80000000; 
  const uint32_t top_n_bits = frac_bits >> (31 - LOG2_LOOKUP_TABLE_SIZE_BITS);

  number_of_bits <<= Q24_FRAC_BITS;
  const unsigned shift_threshold = Q24_FRAC_BITS + 1;
  if(LOG2_LOOKUP_TYPE_BITS > shift_threshold) number_of_bits += log2_lookup[top_n_bits] >> (LOG2_LOOKUP_TYPE_BITS - shift_threshold);
  else number_of_bits += (uint32_t)log2_lookup[top_n_bits] << (shift_threshold - LOG2_LOOKUP_TYPE_BITS);

  return (fixed_s32_t)number_of_bits;
}

void push_peak_power_into_history(adec_state_t *adec_state, float_s32_t peak_phase_power){
  adec_state->peak_power_history_idx++; //Pre increment so this idx holds the latest value
  if (adec_state->peak_power_history_idx == ADEC_PEAK_LINREG_HISTORY_SIZE){
    adec_state->peak_power_history_idx = 0;
    adec_state->peak_power_history_valid = 1;  // is valid when buffer has at least been filled once
  }
  adec_state->peak_power_history[adec_state->peak_power_history_idx] = peak_phase_power;
}

void get_decimated_peak_power_history(float_s32_t peak_power_decimated[ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE], adec_state_t *adec_state){
  //peak_power_decimated array - biggest index is most recent
  //Get index of latest peak energy value
  int source_idx = adec_state->peak_power_history_idx;

  for (int i = 0; i < ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE; i++){
    source_idx += ADEC_PEAK_LINREG_DECIMATE_RATIO; //pre increment so we start with oldest and end up at latest
    source_idx = (source_idx < ADEC_PEAK_LINREG_HISTORY_SIZE) ? source_idx : source_idx - ADEC_PEAK_LINREG_HISTORY_SIZE;
    peak_power_decimated[i] = adec_state->peak_power_history[source_idx];
  }
}

//Specialised version of general linear regression that assumes x values are always 0, 1, 2, etc.
//It also scales the result by a) the decimation ratio and b) the absolute value of the first datapoint
//and so the slope is relative rather than absolute. This normalises it which is important when mag(H) is unknown
float_s32_t linear_regression_get_slope(float_s32_t pk_energies[], unsigned n, float_s32_t scale_factor){
  float_s32_t sumx, sumxsq, sumy, sumxy;
  float_s32_t zero = double_to_float_s32(0.0);
  sumx = zero;
  sumxsq = zero;
  sumy = zero;
  sumxy = zero;

  //Get length of sequence
  float_s32_t len = {n, 0};
  float_s32_t tmp;

  //all indicies and energies are unsigned
  for (unsigned idx = 0; idx < n; idx++){
    float_s32_t i = {idx, 0};

    sumx = float_s32_add(sumx, i); //sumx += i

    tmp = float_s32_mul(i, i);
    sumxsq = float_s32_add(sumxsq, tmp); //sumxq = sumxq + (i*i)

    float_s32_t y_value = pk_energies[idx];
    tmp = float_s32_mul(i, y_value);//sumxy += i * sequence[i]
    sumxy = float_s32_add(tmp, sumxy);

    sumy = float_s32_add(sumy, y_value);//sumy += sequence[i]
    //Unused                                        //sumysq += sequence[i] * sequence[i]
  }

  tmp = float_s32_mul(sumx, sumx);//denom = (len(sequence) * sumxsq - (sumx * sumx)) * scale_factor

  float_s32_t denom;
  denom = float_s32_mul(len, sumxsq);
  denom = float_s32_sub(denom, tmp); 

  denom = float_s32_mul(denom, scale_factor);//Account for decimation
  if(denom.mant == 0)
  {
    return zero;
  }

  tmp = float_s32_mul(sumx, sumy);//slope = ((len(sequence) * sumxy) - (sumx * sumy)) / denom
  float_s32_t slope;
  slope = float_s32_mul(len, sumxy);
  slope = float_s32_sub(slope, tmp);
  slope = float_s32_div(slope, denom);

  //We don't care about intercept                   //intercept = ((sumy * sumxsq) - (sumx * sumxy)) / denom
  return slope;
}


static inline fixed_s32_t multiply_q24_no_saturation(fixed_s32_t a_q24, fixed_s32_t b_q24){
  int64_t result_ll = (int64_t)a_q24 * (int64_t)b_q24;
  return (fixed_s32_t)(result_ll >> 24);
}

//This function modifies the agm (AEC Goodness Metric) according to state of ERLE and the peak power slope
fixed_s32_t calculate_aec_goodness_metric(adec_state_t *state, fixed_s32_t log2erle_q24, float_s32_t peak_power_slope, fixed_s32_t agm_q24){

  //All good if ERLE is high
  if (log2erle_q24 >= state->erle_good_bits_q24){
    // debug_printf("*AGM ERLE ALL GOOD\n");
    state->convergence_counter = 0;
    state->shadow_flag_counter = 0;
    return ADEC_AGM_ONE;
  }

  fixed_s32_t erle_agm_delta_q24 = 0;
  if (log2erle_q24 < state->erle_bad_bits_q24){
    //This value will be negative when dB ERLE is less than 0 (ratio <1). Note it is in Q7.24 format
    erle_agm_delta_q24 = (log2erle_q24 - state->erle_bad_bits_q24); 
    erle_agm_delta_q24 = multiply_q24_no_saturation(erle_agm_delta_q24, FLOAT_TO_Q24(log(10)/log(2))); //Scale from log2 to 10log10
    erle_agm_delta_q24 = multiply_q24_no_saturation(erle_agm_delta_q24, state->erle_bad_gain_q24);
  }

  //Convert to 8.24 format
  bfp_s32_t temp; //Till we get support for float_s32_use_exponent, convert to a BFP array of length 1 and call bfp_s32_use_exponent
  bfp_s32_init(&temp, &peak_power_slope.mant, peak_power_slope.exp, 1, 1);
  bfp_s32_use_exponent(&temp, -Q24_FRAC_BITS);
  peak_power_slope.exp = temp.exp;
  fixed_s32_t peak_slope_q24 = peak_power_slope.mant; //now in 8.24 format
  peak_slope_q24 = multiply_q24_no_saturation(peak_slope_q24, state->peak_phase_energy_trend_gain_q24); 
  
  fixed_s32_t new_agm_q24 = (agm_q24 + erle_agm_delta_q24 + peak_slope_q24);
  //printf("new_agm %d, agm %d, erle_agm_delta_q24 %d, peak_slope %d\n",new_agm, agm_q24, erle_agm_delta_q24, peak_slope_q24);

  //Clip positive - negative will be captured by mode change logic
  if (new_agm_q24 > ADEC_AGM_ONE){
    new_agm_q24 = ADEC_AGM_ONE;
    state->convergence_counter = 0;
    state->shadow_flag_counter = 0;
  }
  return new_agm_q24;
}

void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const adec_input_t *adec_in
){
  adec_output->reset_all_aec_flag = 0;
  adec_output->delay_change_request_flag = 0;
  adec_output->delay_estimator_enabled_flag = (state->mode == ADEC_NORMAL_AEC_MODE) ? 0 : 1;
  adec_output->requested_mic_delay_samples = 0;

  uint32_t elapsed_milliseconds = adec_in->num_frames_since_last_call*15; //Each frame is 15ms

  const float_s32_t aec_peak_to_average_good_de_threshold       = ADEC_PEAK_TO_AVERAGE_GOOD_DE;
  const float_s32_t aec_peak_to_average_ruined_aec_threshold    = ADEC_PEAK_TO_AVERAGE_RUINED_AEC;
 
  //The XC AEC, despite being reset, sometimes starts up with some odd phase energies which make it
  //appear there is a strong pk:ave before it converges. These erode as it converges and a genuine peak
  //forms later. So we have logic to ensure that the pk:ave is not descending at first 
  //before rising back up. We apply a some filtering (moving average) to reduce noise and init to a large value (1000),
  //so we are sure the numbers drop at first.
  //Previously we used a total phase energy threshold but this was difficult to set (depends on relative far/near levels).
  //Following that a hysteresis was used but that sometimes didn't trigger, so now we look for a +ve trend.
  //This all takes 892 cyc in the sim which could be 1427 cyc in device (62.5MHz) so about 14us  

  // Shift them along
  for(int i = ADEC_PEAK_TO_RAGE_HISTORY_DEPTH; i > 0; i--){
    state->peak_to_average_ratio_history[i] = state->peak_to_average_ratio_history[i - 1];
  }
  state->peak_to_average_ratio_history[0] = adec_in->from_de.peak_to_average_ratio;

  float_s32_t last_n_total = double_to_float_s32(0.0);
  for(int i = 0; i < ADEC_PEAK_TO_RAGE_HISTORY_DEPTH; i++){
    last_n_total = float_s32_add(last_n_total, state->peak_to_average_ratio_history[i]);
  }
 
  float_s32_t penultimate_n_total = double_to_float_s32(0.0);
  for(int i = 1; i < ADEC_PEAK_TO_RAGE_HISTORY_DEPTH + 1; i++){
    penultimate_n_total = float_s32_add(penultimate_n_total, state->peak_to_average_ratio_history[i]);
  }

  // Note we look for last_n_total > penultimate_n_total so !(penultimate_n_total >= last_n_total)
  // As we want upwards trend not flat
  if (!float_s32_gte(penultimate_n_total, last_n_total)){
    if (state->peak_to_average_ratio_valid_flag != 1) printf("***WERE ON THE UP MY FRIEND.. ***\n");
    state->peak_to_average_ratio_valid_flag = 1;
  }
  else{
    // Still descending. Wait for now.
  }

  //Log the biggest peak:ave ratio since AEC reset - gives inidication of convergence
  if (float_s32_gte(adec_in->from_de.peak_to_average_ratio, state->max_peak_to_average_ratio_since_reset) 
    && (state->peak_to_average_ratio_valid_flag == 1)){
    state->max_peak_to_average_ratio_since_reset = adec_in->from_de.peak_to_average_ratio;
  }

  //Work out the trend (slope) of the peak phase power to see if we are diverging..
  push_peak_power_into_history(state, adec_in->from_de.peak_phase_power);
  float_s32_t peak_power_decimated[ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE];
  get_decimated_peak_power_history(peak_power_decimated, state);
  float_s32_t decimate_ratio = {ADEC_PEAK_LINREG_DECIMATE_RATIO, 0};
  float_s32_t decimate_norm_product = float_s32_mul(decimate_ratio, peak_power_decimated[ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE - 1]);
  float_s32_t peak_power_slope = double_to_float_s32(0.0);
  // Only calculate slope from linear regression if the history is valid
  if (state->peak_power_history_valid){
    peak_power_slope= linear_regression_get_slope(peak_power_decimated, ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE, decimate_norm_product);
  }

  //Work out erle in log2
  float_s32_t erle_ratio;
  float_s32_t denom = adec_in->from_aec.error_ema_energy_ch0;
  if(denom.mant != 0) {
      erle_ratio = float_s32_div(adec_in->from_aec.y_ema_energy_ch0, denom);
  }
  else {
      erle_ratio = double_to_float_s32(1.0);
  }
  fixed_s32_t log2erle_q24 = float_to_frac_bits(erle_ratio);

  switch(state->mode){
    case(ADEC_NORMAL_AEC_MODE):
        if (adec_in->from_aec.shadow_better_or_equal_flag) {
          reset_stuff_on_AEC_mode_start(state, 0);
          ++state->shadow_flag_counter;
          state->convergence_counter = 0;
        } else {
          ++state->convergence_counter;
        }

        if (adec_in->from_aec.shadow_to_main_copy_flag) {
          state->sf_copy_flag = 1;
        }

        //In normal AEC mode, check to see if we have converged but have left significant tail on the table
        //But only change mode if the delay change is big enough - else reset of AEC not worth it
        if ((state->gated_milliseconds_since_mode_change > ADEC_AEC_DELAY_EST_TIME_MS) &&
          (float_s32_gte(adec_in->from_de.peak_to_average_ratio, state->aec_peak_to_average_good_aec_threshold)) &&
          (state->peak_to_average_ratio_valid_flag == 1) &&
          (adec_in->from_de.delay_estimate > MILLISECONDS_TO_SAMPLES(ADEC_AEC_ESTIMATE_MIN_MS)) &&
          (!state->adec_config.bypass)){

          //We have a new estimate RELATIVE to current delay settings
          state->last_measured_delay += adec_in->from_de.delay_estimate;
          printf("AEC MODE - Measured delay estimate: %ld (raw %ld)\n", state->last_measured_delay, adec_in->from_de.delay_estimate); //+ve means MIC delay
          set_delay_params_from_signed_delay(state->last_measured_delay, &adec_output->requested_mic_delay_samples, &state->measured_delay_samples_debug);
          adec_output->reset_all_aec_flag = 1;
          reset_stuff_on_AEC_mode_start(state, 1);
          state->mode = state->mode; //Same mode (no change)

          //printf("Mode Change requested 1\n");          
          adec_output->delay_change_request_flag = 1; //But we want to reset AEC even though we are staying in the same mode
          break;
        }

        //Only do DEC logic if we have a significant amount of far end energy else AEC stats may not be useful
        if (adec_in->far_end_active_flag && state->sf_copy_flag){

          //AEC goodness calculation
          state->agm_q24 = calculate_aec_goodness_metric(state, log2erle_q24, peak_power_slope, state->agm_q24);

          //Action if force trigger or if agm dips below zero or watchdog when adec enabled - things are totally ruined so do full delay estimate cycle
          if(float_s32_gte(aec_peak_to_average_ruined_aec_threshold, adec_in->from_de.peak_to_average_ratio)) printf("less than 2\n");
          unsigned watchdog_triggered = (
                       (state->gated_milliseconds_since_mode_change > (ADEC_PK_AVE_POOR_WATCHDOG_SECONDS * 1000))
                      && ((float_s32_gte(state->aec_peak_to_average_good_aec_threshold, state->max_peak_to_average_ratio_since_reset)) ||
                          ((state->peak_to_average_ratio_valid_flag == 1) && (float_s32_gte(aec_peak_to_average_ruined_aec_threshold, adec_in->from_de.peak_to_average_ratio)) )
                         )
                                        );

          //Do some debug printing if the watchdog has triggered.
          //TODO Fix debug prints
          /*if (watchdog_triggered && !state->adec_config.bypass){
            printf("Watchdog, max_pk_ave_b: %d thresh: %d time: %d limit: %d\n",
              float_to_frac_bits(state->max_peak_to_average_ratio_since_reset),
              float_to_frac_bits(state->aec_peak_to_average_good_aec_threshold),
              state->gated_milliseconds_since_mode_change,
              (ADEC_PK_AVE_POOR_WATCHDOG_SECONDS * 1000));
          }*/

          //printf("state->agm = %d, watchdog_triggered = %d, state->shadow_flag_counter = %d, state->convergence_counter = %d\n", state->agm_q24, watchdog_triggered, state->shadow_flag_counter, state->convergence_counter);
          
          if (state->adec_config.force_de_cycle_trigger ||
              ((state->agm_q24 < 0 || watchdog_triggered) &&
               (state->shadow_flag_counter >= ADEC_SHADOW_FLAG_COUNTER_LIMIT ||
                state->convergence_counter >= ADEC_CONVERGENCE_COUNTER_LIMIT))) {
		  
            if (!state->adec_config.bypass || state->adec_config.force_de_cycle_trigger) {
              if (state->adec_config.force_de_cycle_trigger) {
                if (state->agm_q24 < 0) state->agm_q24 = 0;//clip negative
                printf("force_de_cycle_trigger\n");
              }

              //AEC is ruined so switch on control flag for DE, stage_a logic will handle mode transition nicely.
              //But first we need to set the delay value so we can see forwards/backwards, see configure_delay.py
              adec_output->requested_mic_delay_samples = ADEC_DE_DELAY_OFFSET_SAMPS;
              adec_output->delay_estimator_enabled_flag = 1;

              state->mode = ADEC_DELAY_ESTIMATOR_MODE;
              state->gated_milliseconds_since_mode_change = 0;
              //printf("Mode Change requested 2\n");              
              adec_output->delay_change_request_flag = 1;
            }
          }
        }
        break;

    case(ADEC_DELAY_ESTIMATOR_MODE):
        //track peak to average ratio and minimum time with far end energy to know when to change
        // printf("peak:ave * 1024: %d\n", vtb_denormalise_and_saturate_u32(state->peak_to_average_ratio, -10));
        if ((state->gated_milliseconds_since_mode_change > ADEC_DELAY_EST_MODE_TIME_MS) &&
          float_s32_gte(adec_in->from_de.peak_to_average_ratio, aec_peak_to_average_good_de_threshold) &&
          (state->peak_to_average_ratio_valid_flag == 1)){

          //We have come from DE mode with a new estimate and need to reset AEC + adjust delay
          //so switch back to AEC normal mode + set delay from fresh
          state->last_measured_delay = adec_in->from_de.delay_estimate - MAX_DELAY_SAMPLES;
          printf("DE MODE - Measured delay estimate: %ld (raw %ld)\n", state->last_measured_delay, adec_in->from_de.delay_estimate); //+ve means MIC delay
          //printf("pkave bits: %d, val * 1024: %d\n", vtb_u32_float_to_bits(adec_in->from_de.peak_to_average_ratio), vtb_denormalise_and_saturate_u32(adec_in->from_de.peak_to_average_ratio, -10));

          set_delay_params_from_signed_delay(state->last_measured_delay, &adec_output->requested_mic_delay_samples, &state->measured_delay_samples_debug);

          state->mode = ADEC_NORMAL_AEC_MODE;
          adec_output->delay_estimator_enabled_flag = 0;
          reset_stuff_on_AEC_mode_start(state, 1);
          //printf("Mode Change requested 3\n");
           
          adec_output->delay_change_request_flag = 1;
        }
      break;

      default:
        __builtin_unreachable();
      break;
    }

    if (adec_in->far_end_active_flag) {
      state->gated_milliseconds_since_mode_change += elapsed_milliseconds;
    }

  if (adec_output->delay_change_request_flag == 1){ //TODO Is this needed when already calling reset_stuff_on_AEC_mode_start()?
      state->gated_milliseconds_since_mode_change = 0;
      state->max_peak_to_average_ratio_since_reset = double_to_float_s32(1.0);
      state->peak_to_average_ratio_valid_flag = 0;
      init_pk_ave_ratio_history(state);
  }

  return;
}
