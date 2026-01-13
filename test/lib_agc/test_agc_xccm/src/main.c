// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "agc.h"

int main()
{
  agc_config_t conf = AGC_PROFILE_ASR;
  agc_state_t agc;
  agc_init(&agc, &conf);

  agc_meta_data_t md = {AGC_META_DATA_NO_VNR, AGC_META_DATA_NO_AEC, AGC_META_DATA_NO_AEC};
  int32_t input[AGC_FRAME_ADVANCE] = {0};
  int32_t output[AGC_FRAME_ADVANCE] = {0};

  for (unsigned i = 0; i < 10; i++)
  {
    agc_process_frame(&agc, output, input, &md);
    printf("%ld, ", output[0]);
  }
  printf("\n");
  return 0;
}
