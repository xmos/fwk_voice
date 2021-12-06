// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AEC_MEMORY_POOL_H
#define AEC_MEMORY_POOL_H

#include "aec_config.h"
//Memory pool definition
typedef struct {
    int32_t mic_input_frame[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];
    int32_t ref_input_frame[AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH + 2];
    int32_t mic_prev_samples[AEC_MAX_Y_CHANNELS][AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE]; //272 samples history
    int32_t ref_prev_samples[AEC_MAX_X_CHANNELS][AEC_PROC_FRAME_LENGTH - AEC_FRAME_ADVANCE]; //272 samples history
    complex_s32_t phase_pool_H_hat_X_fifo[((AEC_MAX_Y_CHANNELS*AEC_MAX_X_CHANNELS*AEC_MAIN_FILTER_PHASES) + (AEC_MAX_X_CHANNELS*AEC_MAIN_FILTER_PHASES)) * (AEC_PROC_FRAME_LENGTH/2 + 1)];
    complex_s32_t Error[AEC_MAX_Y_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    complex_s32_t Y_hat[AEC_MAX_Y_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    int32_t X_energy[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    int32_t sigma_XX[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    int32_t inv_X_energy[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2)+1];
    int32_t output[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
    int32_t overlap[AEC_MAX_Y_CHANNELS][UNUSED_TAPS_PER_PHASE*2];
}aec_memory_pool_t;

typedef struct {
    complex_s32_t phase_pool_H_hat[AEC_MAX_Y_CHANNELS * AEC_MAX_X_CHANNELS * AEC_SHADOW_FILTER_PHASES * (AEC_PROC_FRAME_LENGTH/2 + 1)];
    complex_s32_t Error[AEC_MAX_Y_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    complex_s32_t Y_hat[AEC_MAX_Y_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    complex_s32_t T[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    int32_t X_energy[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2) + 1];
    int32_t inv_X_energy[AEC_MAX_X_CHANNELS][(AEC_PROC_FRAME_LENGTH/2)+1];
    int32_t output[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
    int32_t overlap[AEC_MAX_Y_CHANNELS][UNUSED_TAPS_PER_PHASE*2];
}aec_shadow_filt_memory_pool_t;
#endif
