#ifndef __VNR_FEATURES_STATE_H__
#define __VNR_FEATURES_STATE_H__

#include "bfp_math.h"
#include "xs3_math.h"

#define VNR_PROC_FRAME_LENGTH (512)
#define VNR_FRAME_ADVANCE (240)
#define VNR_INPUT_EXP (-31)
#define VNR_FFT_PADDING (2)
#define VNR_FD_FRAME_LENGTH ((VNR_PROC_FRAME_LENGTH/2)+1)

typedef struct {
    int32_t DWORD_ALIGNED prev_input_samples[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE];
}vnr_input_state_t;
#endif
