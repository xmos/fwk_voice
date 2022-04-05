#ifndef __VNR_STATE_H__
#define __VNR_STATE_H__

#include "vnr_model_data.h"
#include "xtflm_conf.h"
#include "inference_engine.h"
#include "xs3_math.h"

#define TENSOR_ARENA_SIZE_BYTES 100000
#define VNR_PROC_FRAME_LENGTH (512)
#define VNR_FRAME_ADVANCE (240)
#define VNR_INPUT_EXP (-31)
#define VNR_FFT_PADDING (2)
typedef struct {
    inference_engine_t ie; 
    uint64_t tensor_arena[TENSOR_ARENA_SIZE_BYTES/sizeof(uint64_t)];
    int8_t *input_buffer;
    size_t input_size;
    int8_t *output_buffer;
    size_t output_size;
}vnr_ie_t;

typedef struct {
    int32_t DWORD_ALIGNED prev_input_samples[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE];
}vnr_input_state_t;

#endif
