// Copyright (c) 2017-2018, XMOS Ltd, All rights reserved
#ifndef APP_PARAMETERS_H
#define APP_PARAMETERS_H

#define VAD_N_FEATURES_PER_FRAME                           26
#define VAD_FRAMES_PER_CLASSIFY                             4
#define VAD_FRAME_STRIDE                                    3
#define VAD_N_FEATURES   (VAD_N_FEATURES_PER_FRAME * VAD_FRAMES_PER_CLASSIFY)
#define VAD_N_OLD_FEATURES ((VAD_N_FEATURES_PER_FRAME) * ((VAD_FRAMES_PER_CLASSIFY) * (VAD_FRAME_STRIDE)-1) + 1)
#define VAD_LOG_WINDOW_LENGTH                               9
#define VAD_WINDOW_LENGTH          (1<<VAD_LOG_WINDOW_LENGTH)
#define VAD_DSP_SINE_WINDOW_LENGTH               dsp_sine_512

#define VAD_STATE_LENGTH()               (VAD_N_OLD_FEATURES)
#define VAD_PROC_FRAME_LENGTH VAD_WINDOW_LENGTH
#define VAD_FRAME_ADVANCE 240
    
#endif
