#include <string.h>
#include <math.h>
#include <limits.h>
#include "adec_api.h"
#include "adec_priv.h"
#include "aec_state.h" // For shadow_state_e enum


void adec_init(adec_state_t *adec_state, adec_config_t *config){
  adec_state->adec_config.bypass = config->bypass;
  adec_state->adec_config.force_de_cycle_trigger = config->force_de_cycle_trigger;
  adec_state->agm_q24 = ADEC_AGM_HALF;

  //Using bits log2(erle) with q7_28 gives us up to 10log(2^127) = 382dB ERLE measurement range.. 
  adec_state->erle_bad_bits_q24 = FLOAT_TO_Q24(ADEC_ERLE_BAD_BITS);
  adec_state->erle_good_bits_q24 = FLOAT_TO_Q24(ADEC_ERLE_GOOD_BITS);
  adec_state->peak_phase_energy_trend_gain_q24 = FLOAT_TO_Q24(ADEC_PEAK_PHASE_ENERGY_TREND_GAIN);
  adec_state->erle_bad_gain_q24 = FLOAT_TO_Q24(ADEC_ERLE_BAD_GAIN);

  adec_state->peak_to_average_ratio_valid_flag = 0;
  adec_state->max_peak_to_average_ratio_since_reset = f64_to_float_s32(1.0);

  float_s32_t v = ADEC_PEAK_TO_AVERAGE_GOOD_AEC;
  adec_state->aec_peak_to_average_good_aec_threshold = v; 

  adec_state->mode = ADEC_NORMAL_AEC_MODE;
  adec_state->gated_milliseconds_since_mode_change = 0;
  adec_state->last_measured_delay = 0;

  for (int i = 0; i < ADEC_PEAK_LINREG_HISTORY_SIZE; i++){
    adec_state->peak_power_history[i] = f64_to_float_s32(0.0);
  }
  adec_state->peak_power_history_idx = 0;
  adec_state->peak_power_history_valid = 0;

  init_pk_ave_ratio_history(adec_state);

  adec_state->sf_copy_flag = 0;
  adec_state->convergence_counter = 0;
  adec_state->shadow_flag_counter = 0;
}

static void start_de_cycle(adec_state_t *state, adec_output_t *adec_output) {
    // Request transition to DE mode from stage 1.
    // Set the delay value so we can see forwards/backwards
    adec_output->requested_mic_delay_samples = ADEC_DE_DELAY_OFFSET_SAMPS;
    adec_output->delay_estimator_enabled_flag = 1;

    state->mode = ADEC_DELAY_ESTIMATOR_MODE;
    state->gated_milliseconds_since_mode_change = 0;
    adec_output->delay_change_request_flag = 1;
}

void adec_process_frame(
    adec_state_t *state,
    adec_output_t *adec_output, 
    const adec_input_t *adec_in
){
  adec_output->reset_aec_flag = 0;
  adec_output->delay_change_request_flag = 0;
  adec_output->delay_estimator_enabled_flag = (state->mode == ADEC_NORMAL_AEC_MODE) ? 0 : 1;

  uint32_t elapsed_milliseconds = 15; //Each frame is 15ms and assuming adec process frame is called every frame since that's the only mode supported

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
  for(int i = ADEC_PEAK_TO_AVERAGE_HISTORY_DEPTH; i > 0; i--){
    state->peak_to_average_ratio_history[i] = state->peak_to_average_ratio_history[i - 1];
  }
  state->peak_to_average_ratio_history[0] = adec_in->from_de.peak_to_average_ratio;

  float_s32_t last_n_total = f64_to_float_s32(0.0);
  for(int i = 0; i < ADEC_PEAK_TO_AVERAGE_HISTORY_DEPTH; i++){
    last_n_total = float_s32_add(last_n_total, state->peak_to_average_ratio_history[i]);
  }
 
  float_s32_t penultimate_n_total = f64_to_float_s32(0.0);
  for(int i = 1; i < ADEC_PEAK_TO_AVERAGE_HISTORY_DEPTH + 1; i++){
    penultimate_n_total = float_s32_add(penultimate_n_total, state->peak_to_average_ratio_history[i]);
  }

  // Note we look for last_n_total > penultimate_n_total so !(penultimate_n_total >= last_n_total)
  // As we want upwards trend not flat
  if (!float_s32_gte(penultimate_n_total, last_n_total)){
    if (state->peak_to_average_ratio_valid_flag != 1) {
#ifdef ENABLE_ADEC_DEBUG_PRINTS
        printf("***WERE ON THE UP MY FRIEND.. ***\n");
#endif
    }
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
  float_s32_t peak_power_slope = f64_to_float_s32(0.0);
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
      erle_ratio = f64_to_float_s32(1.0);
  }
  //printf("erle_ratio = %f\n",float_s32_to_float(erle_ratio));
  q8_24 log2erle_q24 = float_to_frac_bits(erle_ratio);

  switch(state->mode){
    case(ADEC_NORMAL_AEC_MODE):
        if (adec_in->from_aec.shadow_flag_ch0 > EQUAL) {
          reset_stuff_on_AEC_mode_start(state, 0);
          ++state->shadow_flag_counter;
          state->convergence_counter = 0;
        } else {
          ++state->convergence_counter;
        }

        if (adec_in->from_aec.shadow_flag_ch0 == COPY) {
          state->sf_copy_flag = 1;
        }
        
        //In normal AEC mode, check to see if we have converged but have left significant tail on the table
        //But only change mode if the delay change is big enough - else reset of AEC not worth it
        if ((state->gated_milliseconds_since_mode_change > ADEC_AEC_DELAY_EST_TIME_MS) &&
          (float_s32_gte(adec_in->from_de.peak_to_average_ratio, state->aec_peak_to_average_good_aec_threshold)) &&
          (state->peak_to_average_ratio_valid_flag == 1) &&
          (adec_in->from_de.measured_delay_samples > MILLISECONDS_TO_SAMPLES(ADEC_AEC_ESTIMATE_MIN_MS)) &&
          (!state->adec_config.bypass)){

          //We have a new estimate RELATIVE to current delay settings
          state->last_measured_delay += adec_in->from_de.measured_delay_samples;
#ifdef ENABLE_ADEC_DEBUG_PRINTS
          printf("AEC MODE - Measured delay estimate: %ld (raw %ld)\n", state->last_measured_delay, adec_in->from_de.delay_estimate); //+ve means MIC delay
#endif
          set_delay_params_from_signed_delay(state->last_measured_delay, &adec_output->requested_mic_delay_samples, &adec_output->requested_delay_samples_debug);
          adec_output->reset_aec_flag = 1;
          state->mode = state->mode; //Same mode (no change)

          adec_output->delay_change_request_flag = 1; //But we want to reset AEC even though we are staying in the same mode
          break;
        }

        if (adec_in->far_end_active_flag && state->adec_config.force_de_cycle_trigger) // If configured to force a DE cycle, switch to DE mode without waiting for shadow filter copy
        {
            if (state->agm_q24 < 0) {
                state->agm_q24 = 0;//clip negative
            }
#ifdef ENABLE_ADEC_DEBUG_PRINTS
            printf("force_de_cycle_trigger\n");
#endif
            // Trigger a DE cycle
            start_de_cycle(state, adec_output);
            break;
        }

        //Only do DEC logic if we have a significant amount of far end energy else AEC stats may not be useful
        if (adec_in->far_end_active_flag && state->sf_copy_flag){

          //AEC goodness calculation
          state->agm_q24 = calculate_aec_goodness_metric(state, log2erle_q24, peak_power_slope, state->agm_q24);

          //Action if force trigger or if agm dips below zero or watchdog when adec enabled - things are totally ruined so do full delay estimate cycle
          if(float_s32_gte(aec_peak_to_average_ruined_aec_threshold, adec_in->from_de.peak_to_average_ratio)) {
#ifdef ENABLE_ADEC_DEBUG_PRINTS
              printf("less than 2\n");
#endif
          }
          unsigned watchdog_triggered = (
                       (state->gated_milliseconds_since_mode_change > (ADEC_PK_AVE_POOR_WATCHDOG_SECONDS * 1000))
                      && ((float_s32_gte(state->aec_peak_to_average_good_aec_threshold, state->max_peak_to_average_ratio_since_reset)) ||
                          ((state->peak_to_average_ratio_valid_flag == 1) && (float_s32_gte(aec_peak_to_average_ruined_aec_threshold, adec_in->from_de.peak_to_average_ratio)) )
                         )
                                        );

          if ((state->agm_q24 < 0 || watchdog_triggered) &&
               (state->shadow_flag_counter >= ADEC_SHADOW_FLAG_COUNTER_LIMIT ||
                state->convergence_counter >= ADEC_CONVERGENCE_COUNTER_LIMIT)) {
		  
            if (!state->adec_config.bypass) {
                // Trigger a DE cycle
                start_de_cycle(state, adec_output);
            }
          }
        }
        break;

    case(ADEC_DELAY_ESTIMATOR_MODE):
        //track peak to average ratio and minimum time with far end energy to know when to change
        if ((state->gated_milliseconds_since_mode_change > ADEC_DELAY_EST_MODE_TIME_MS) &&
          float_s32_gte(adec_in->from_de.peak_to_average_ratio, aec_peak_to_average_good_de_threshold) &&
          (state->peak_to_average_ratio_valid_flag == 1)){

          //We have come from DE mode with a new estimate and need to reset AEC + adjust delay
          //so switch back to AEC normal mode + set delay from fresh
          state->last_measured_delay = adec_in->from_de.measured_delay_samples - MAX_DELAY_SAMPLES;
#ifdef ENABLE_ADEC_DEBUG_PRINTS
          printf("DE MODE - Measured delay estimate: %ld (raw %ld)\n", state->last_measured_delay, adec_in->from_de.delay_estimate); //+ve means MIC delay
#endif
          set_delay_params_from_signed_delay(state->last_measured_delay, &adec_output->requested_mic_delay_samples, &adec_output->requested_delay_samples_debug);
          state->mode = ADEC_NORMAL_AEC_MODE;
          adec_output->delay_estimator_enabled_flag = 0;
           
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

  if (adec_output->delay_change_request_flag == 1){
      reset_stuff_on_AEC_mode_start(state, 1);
  }
}
