// Copyright 2019-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define ADEC_CONFIG_DE_ONLY_AT_STARTUP (adec_config_t){ \
    .bypass = 1, \
    .force_de_cycle_trigger = 1 \
}

#define ADEC_CONFIG_AUTOMATIC_DE_CONTROL (adec_config_t){ \
    .bypass = 0, \
    .force_de_cycle_trigger = 0 \
}

#define MAX_DELAY_MS                ( 150 )
#define MAX_DELAY_SAMPLES           ( 16000*MAX_DELAY_MS/1000 )

#define ADEC_AGM_ONE                            (1<<24)           //Q24 1.0 MAX AGM
#define ADEC_AGM_HALF                           (1<<23)           //Q24 0.5 INITIAL AGM
#define ADEC_ERLE_BAD_BITS                      -0.066            //-0.2dB -> 0.95 -> -0.066b
#define ADEC_ERLE_GOOD_BITS                     2.0              //6dB
#define ADEC_ERLE_BAD_GAIN                      0.0625           //When ERLE goes below the threshold, how steep is the curve that reduces goodness
#define ADEC_PEAK_PHASE_ENERGY_TREND_GAIN       3.0              //How sensitve we are to the peak phase energy trend, multiplier
#define ADEC_PEAK_TO_AVERAGE_GOOD_AEC           float_to_float_s32(4.0)//Denormalised 4.0 - AEC only has 10 phases so pk:ave 
#define ADEC_PEAK_TO_AVERAGE_GOOD_DE            float_to_float_s32(8.0)//Denormalised 8.0 - With 30 phases, it's easier to get a strong pk
#define ADEC_PEAK_TO_AVERAGE_RUINED_AEC         float_to_float_s32(2.0)//Denormalised 2.0 - Used with watchdog.

#define ADEC_PEAK_TO_RAGE_HISTORY_DEPTH         8                //How far we look back to smooth the pk:ave ratio history

#define ADEC_PK_AVE_POOR_WATCHDOG_SECONDS       30               //How long before we trigger DE mode if we haven't reached a good state after AEC reset

#define ADEC_MODEL_TO_XCORE_SCALE_BITS          8                //TODO - work out why xcore phase magnitudes are this much bigger than the model

#define ADEC_AEC_DELAY_EST_TIME_MS              5000             //How long after mode change with far end present we need to be sure AEC is converged
#define ADEC_DELAY_EST_MODE_TIME_MS             3000             //How long in delay estimator mode with far end present we need to be sure DE is converged
#define ADEC_AEC_ESTIMATE_MIN_MS                20               //Do not restart AEC if delay is smaller than this (no point as we have most of tail anyhow)
#define ADEC_DE_DELAY_OFFSET_SAMPS              (MAX_DELAY_SAMPLES - 1)//Delay offset applied before we run delay estimator for correct window positioning
#define ADEC_DE_DELAY_HEADROOM_SAMPS            240              //Number of samples padding to ensure that mics are always after reference. 240 = one phase
#define ADEC_PEAK_LINREG_DECIMATE_RATIO         11               //Reduce computation when calculating slope, must be a factor of ADEC_PEAK_LINREG_HISTORY_SIZE
#define ADEC_PEAK_LINREG_HISTORY_SIZE           66               //Size of the history buffer. Must be divisible by ADEC_PEAK_LINREG_DECIMATE_RATIO

#define ADEC_CONVERGENCE_COUNTER_LIMIT          (66*5)           //Number of samples to hold off the ADEC while the shadow filter attempts to converge
#define ADEC_SHADOW_FLAG_COUNTER_LIMIT          3                //Number of times shadow filter needs to be better before calculating new delay

#define MILLISECONDS_TO_SAMPLES(ms) (ms * (16000 / 1000))

#if (ADEC_PEAK_LINREG_HISTORY_SIZE % ADEC_PEAK_LINREG_DECIMATE_RATIO)
#error ADEC_PEAK_LINREG_DECIMATE_RATIO must be a factor of ADEC_PEAK_LINREG_HISTORY_SIZE
#endif
#define ADEC_PEAK_LINREG_HISTORY_DECIMATED_SIZE (ADEC_PEAK_LINREG_HISTORY_SIZE/ADEC_PEAK_LINREG_DECIMATE_RATIO)

#define Q24_FRAC_BITS                 24

#define FLOAT_TO_Q24(f) (fixed_s32_t)((float)f * (float)(1 << Q24_FRAC_BITS))

//Fast log2 defines
#define LOG2_LOOKUP_TABLE_SIZE_BITS  8
#define LOG2_LOOKUP_TYPE_BITS 8

#define LOG2_LOOKUP_TABLE_SIZE  (1 << LOG2_LOOKUP_TABLE_SIZE_BITS)
#if (LOG2_LOOKUP_TYPE_BITS == 8)
#define LOG2_LOOKUP_TYPE uint8_t
#elif (LOG2_LOOKUP_TYPE_BITS == 16)
#define LOG2_LOOKUP_TYPE uint16_t
#elif (LOG2_LOOKUP_TYPE_BITS == 32)
#define LOG2_LOOKUP_TYPE uint32_t
#else
#error INVALID LOOKUP TABLE DATA TYPE
#endif
