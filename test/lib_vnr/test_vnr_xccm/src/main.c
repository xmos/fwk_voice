// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "vnr.h"

int main()
{
  vnr_state_t vnr;
  vnr_state_init(&vnr);
  float_s32_t vnr_out;
  int32_t input[VNR_FRAME_ADVANCE] = {0};

  for (unsigned i = 0; i < 10; i++)
  {
    vnr_process_frame(&vnr, &vnr_out, input);
    float res = float_s32_to_float(vnr_out);
    printf("%f, ", res);
  }
  printf("\n");
  return 0;
}
