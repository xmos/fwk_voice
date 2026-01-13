// Copyright 2026 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ns.h"

int main()
{
  ns_state_t ns;
  ns_init(&ns);

  int32_t input[NS_FRAME_ADVANCE] = {0};
  int32_t output[NS_FRAME_ADVANCE] = {0};

  for (unsigned i = 0; i < 10; i++)
  {
    ns_process_frame(&ns, output, input);
    printf("%ld, ", output[0]);
  }
  printf("\n");
  return 0;
}
