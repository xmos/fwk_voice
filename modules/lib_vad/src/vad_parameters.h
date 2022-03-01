// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef VAD_PARAMETERS_H
#define VAD_PARAMETERS_H

#define VAD_N_FEATURES_PER_FRAME                           26
#define VAD_FRAMES_PER_CLASSIFY                             4
#define VAD_FRAME_STRIDE                                    3
#define VAD_N_FEATURES   (VAD_N_FEATURES_PER_FRAME * VAD_FRAMES_PER_CLASSIFY)
#define VAD_N_OLD_FEATURES ((VAD_N_FEATURES_PER_FRAME) * ((VAD_FRAMES_PER_CLASSIFY) * (VAD_FRAME_STRIDE)-1) + 1)
#define VAD_LOG_WINDOW_LENGTH                               9
#define VAD_WINDOW_LENGTH          (1<<VAD_LOG_WINDOW_LENGTH)
#define VAD_DSP_SINE_WINDOW_LENGTH               dsp_sine_512

#define VAD_PROC_FRAME_LENGTH VAD_WINDOW_LENGTH
#define VAD_PROC_FRAME_BINS (VAD_PROC_FRAME_LENGTH / 2)
#define VAD_FRAME_ADVANCE 240
#define VAD_EXP -31

//Taken from ai_nn.h since we removed lib_ai as a dependancy
#define VAD_AI_NN_WEIGHT_Q   24
#define VAD_AI_NN_VALUE_Q    24
#define VAD_AI_NN_OUTPUT_Q   (VAD_AI_NN_WEIGHT_Q + VAD_AI_NN_VALUE_Q)
    
#endif //VAD_PARAMETERS_H
