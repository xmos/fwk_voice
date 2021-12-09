// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _suppression_conf_h
#define _suppression_conf_h

#define SUP_X_CHANNELS              (0)
#define SUP_Y_CHANNELS              (2) // 1 means noise suppression applied only to ASR channel
#define SUP_PROC_FRAME_LENGTH_LOG2  (9)
#define SUP_FRAME_ADVANCE           (240)
#define SUP_PROC_FRAME_LENGTH      (1<<SUP_PROC_FRAME_LENGTH_LOG2) ////////////////2^9 = 512
#define SUP_PROC_FRAME_BINS_LOG2   (SUP_PROC_FRAME_LENGTH_LOG2 - 1)
#define SUP_PROC_FRAME_BINS        (1<<SUP_PROC_FRAME_BINS_LOG2)///////////////////////2^8 = 256
#define INT_EXP (-31)
#define SUPPRESSION_WINDOW_LENGTH (480)

//#define SUP_DEBUG 1
//#define SUP_DEBUG_PRINT 1
//#define SUP_WARNING_PRINT 1


#endif