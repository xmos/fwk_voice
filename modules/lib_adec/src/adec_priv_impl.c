#include <string.h>
#include <math.h>
#include <limits.h>
#include "adec_api.h"
#include "adec_priv.h"

/* About delay:
 * Delay samples refer to mic samples delay. Positive delay means late, Negative delay means early.
 * Positive measured delay means mic is late, so adec will request mic to be moved back in time, which means adec requests
 * a negative mic delay.
 * Negative measured delay means mic is early, so adec will request delaying the mic with a positive requested delay.
 *
 * Whatever is the measured delay, we want to ensure, ADEC requested delay doesn't under any case cause the mic to be
 * early since mic early is the bad case.
 * To ensure that, when measured delay is positive, and ADEC is requesting a mic moving back in time, then we take one phase out of
 * what we're requesting. For eg. measured_delay = 960, requested_delay should be -960 but we request -960+240 = -480 to
 * make sure we don't make the mic early too much risking a mic earlier than ref situation, which results in a filter that is unable to converge due lack of causality. 
 * Similarly, if measured delay is -ve, for example, measured_delay -480 would mean requested_delay +480, but we request
 * 480 + 240 = 960, to avoid mic early case.
 *
 * A filter would never converge when mic is earlier than reference, since none of the phases in X_fifo will match with
 * a given frame of mic data. So in DE mode, in order to measure both mic delay and early case, we start by initially
 * delaying the mic by 10 phases. This means, if there is no actual delay between mic and ref, DE filter will peak at the 10th
 * phase. A peak in the 0-9 phases would measure mic early and a peak in 11-29 phase would indicate mic delay. So in DE mode
 * we can measure upto 10 phases (150ms) of mic early and 20 phases(300ms) of mic delayed. 
 */
void init_pk_ave_ratio_history(adec_state_t *adec_state){
  const float_s32_t float_s32_thousand = {2097160000,-21}; //A large number we never normally see, so it will be clear when we get an actual
  for (int i = 0; i < ADEC_PEAK_TO_AVERAGE_HISTORY_DEPTH; i++){
    adec_state->peak_to_average_ratio_history[i] = float_s32_thousand;
  }
}

void reset_stuff_on_AEC_mode_start(adec_state_t *adec_state, unsigned set_toggle){
  adec_state->agm_q24 = ADEC_AGM_HALF;

  init_pk_ave_ratio_history(adec_state);

  memset(adec_state->peak_power_history, 0, ADEC_PEAK_LINREG_HISTORY_SIZE * sizeof(adec_state->peak_power_history[0]));
  adec_state->peak_power_history_idx = 0;
  adec_state->peak_power_history_valid = 0;

  adec_state->peak_to_average_ratio_valid_flag = 0;
  adec_state->max_peak_to_average_ratio_since_reset = f64_to_float_s32(1.0);

  adec_state->gated_milliseconds_since_mode_change = 0;

  if (set_toggle) {
    adec_state->sf_copy_flag = 0;
    adec_state->shadow_flag_counter = 0;
  }
}

void set_delay_params_from_signed_delay(int32_t measured_delay, int32_t *mic_delay_samples, int32_t *requested_delay_debug){
    //If we see a MIC delay (+ve measured delay), we want to delay Reference by LESS than this to leave some headroom
    //If we see a REF delay (-ve measured delay), we want to delay MIC by MORE than this to leave some headroom
    int32_t measured_delay_compensated = measured_delay - ADEC_DE_DELAY_HEADROOM_SAMPS;

    //Set the requested mic delay to -ve of the measured delay in order to compensate the effect of delay measured in the system
    *mic_delay_samples = -(measured_delay_compensated);
    //Make sure we don't exceed the delay buffer size

    if (*mic_delay_samples >= MAX_DELAY_SAMPLES){
        int32_t actual_delay = MAX_DELAY_SAMPLES - 1; //-1 because we cannot support the actual maximum delay in the buffer (it wraps to zero)
#ifdef ENABLE_ADEC_DEBUG_PRINTS
        printf("**Warning - too large a delay requested (%ld), setting to %ld\n", *mic_delay_samples, actual_delay);
#endif
        *mic_delay_samples = actual_delay;
    }
    else if(*mic_delay_samples <= -(MAX_DELAY_SAMPLES)) {
        int32_t actual_delay = -(MAX_DELAY_SAMPLES - 1); //-1 because we cannot support the actual maximum delay in the buffer (it wraps to zero)
#ifdef ENABLE_ADEC_DEBUG_PRINTS
        printf("**Warning - too large a delay requested (%ld), setting to %ld\n", *mic_delay_samples, actual_delay);
#endif
        *mic_delay_samples = actual_delay;
    }
    //Write to debug copy of var for logging purposes during simulations
    *requested_delay_debug = -(measured_delay_compensated);
}

//This takes about 68 cycles compared with dsp_math_log that takes 12650
//See unit test for const array generation and test. Accuracy is better than 0.06dB with this table & config
int32_t float_to_frac_bits(float_s32_t value){
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
  uint32_t value_mant=0;
  int32_t value_exp=0;
  value_mant = (uint32_t)value.mant << headroom;
  value_exp = value.exp - headroom;

  //Handle case for zero mantissa which is possible with float_s32_t
  if (value_mant == 0) return INT_MIN;

  int32_t number_of_bits = 31 + value_exp;

  const uint32_t frac_bits = (uint32_t)value_mant - (uint32_t)0x80000000; 
  const uint32_t top_n_bits = frac_bits >> (31 - LOG2_LOOKUP_TABLE_SIZE_BITS);

  number_of_bits <<= Q24_FRAC_BITS;
  const unsigned shift_threshold = Q24_FRAC_BITS + 1;
  if(LOG2_LOOKUP_TYPE_BITS > shift_threshold) {
      number_of_bits += (log2_lookup[top_n_bits] >> (LOG2_LOOKUP_TYPE_BITS - shift_threshold));
  }
  else {
      number_of_bits += ((uint32_t)log2_lookup[top_n_bits] << (shift_threshold - LOG2_LOOKUP_TYPE_BITS));
  }

  return (int32_t)number_of_bits;
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
  float_s32_t zero = f64_to_float_s32(0.0);
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


static inline q8_24 multiply_q24_no_saturation(q8_24 a_q24, q8_24 b_q24){
  int64_t result_ll = (int64_t)a_q24 * (int64_t)b_q24;
  return (q8_24)(result_ll >> 24);
}

//This function modifies the agm (AEC Goodness Metric) according to state of ERLE and the peak power slope
q8_24 calculate_aec_goodness_metric(adec_state_t *state, q8_24 log2erle_q24, float_s32_t peak_power_slope, q8_24 agm_q24){

  //All good if ERLE is high
  if (log2erle_q24 >= state->erle_good_bits_q24){
    // debug_printf("*AGM ERLE ALL GOOD\n");
    state->convergence_counter = 0;
    state->shadow_flag_counter = 0;
    return ADEC_AGM_ONE;
  }

  q8_24 erle_agm_delta_q24 = 0;
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
  q8_24 peak_slope_q24 = peak_power_slope.mant; //now in 8.24 format
  peak_slope_q24 = multiply_q24_no_saturation(peak_slope_q24, state->peak_phase_energy_trend_gain_q24); 
  
  q8_24 new_agm_q24 = (agm_q24 + erle_agm_delta_q24 + peak_slope_q24);

  //Clip positive - negative will be captured by mode change logic
  if (new_agm_q24 > ADEC_AGM_ONE){
    new_agm_q24 = ADEC_AGM_ONE;
    state->convergence_counter = 0;
    state->shadow_flag_counter = 0;
  }
  return new_agm_q24;
}
