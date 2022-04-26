#ifndef __VNR_FEATURES_STATE_H__
#define __VNR_FEATURES_STATE_H__

#include "bfp_math.h"
#include "xs3_math.h"

#define VNR_PROC_FRAME_LENGTH (512)
#define VNR_FRAME_ADVANCE (240)
#define VNR_INPUT_EXP (-31)
#define VNR_FFT_PADDING (2)
#define VNR_FD_FRAME_LENGTH ((VNR_PROC_FRAME_LENGTH/2)+1)
#define VNR_MEL_FILTERS (24)
#define VNR_PATCH_WIDTH (4)
#define VNR_LOG2_OUTPUT_EXP (-24)

typedef struct {
    int32_t DWORD_ALIGNED prev_input_samples[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE];
}vnr_input_state_t;

typedef struct {
    fixed_s32_t DWORD_ALIGNED feature_buffers[VNR_PATCH_WIDTH][VNR_MEL_FILTERS];
}vnr_feature_state_t;
#endif
