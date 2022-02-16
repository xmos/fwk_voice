// This file allows old lib_vad mel to compile here
#define dsp_complex_t complex_s32_t
#include "xs3_math_types.h"

void vad_mel_compute_new(int32_t melValues[], uint32_t M,
                           complex_s32_t pts[], uint32_t N,
                           const uint32_t melTable[],
                           int32_t extra_shift) ;
