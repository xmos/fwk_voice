// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "aec.h"

#if CUSTOM_SCHEDULE
extern aec_task_distribution_t tdist;
#endif

int main()
{
  // Initialise AEC
  aec_state_t DWORD_ALIGNED aec_state;
  printf("AEC_MAX_Y_CHANNELS: %d, AEC_MAX_X_CHANNELS: %d, AEC_MAIN_FILTER_PHASES: %d, AEC_SHADOW_FILTER_PHASES: %d\n", AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS, AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES);

#if CUSTOM_SCHEDULE
  aec_task_distribution_t *tdist_ptr = &tdist;
#elif (NUM_THREADS == 2)
  aec_task_distribution_t *tdist_ptr = &aec_tdist_chans2_threads2;
#else
  aec_task_distribution_t *tdist_ptr = &aec_tdist_chans2_threads1;
#endif
  printf("AEC num threads: %d\n", tdist_ptr->thread_count);
  aec_init(&aec_state,
          AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS,
          AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES, tdist_ptr);

  int32_t input_y[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE] = {{0}};
  int32_t input_x[AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE] = {{0}};
  int32_t main_filt_output[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
  int32_t shadow_filt_output[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];

  for (unsigned i = 0; i < 10; i++)
  {
    printf("Frame %d\n", i);
    aec_process_frame(&aec_state, main_filt_output, shadow_filt_output, input_y, input_x);
  }
  printf("\n");
  return 0;
}
