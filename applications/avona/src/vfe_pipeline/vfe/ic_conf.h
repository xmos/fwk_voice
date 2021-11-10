// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef _ic_conf_h
#define _ic_conf_h

#include "voice_front_end_settings.h"

#define IC_PASSTHROUGH_CHANNELS 0
#define IC_PHASES 10
#define IC_FRAME_ADVANCE 240
#define IC_PROC_FRAME_LENGTH_LOG2 VFE_PROC_FRAME_LENGTH_LOG2
#define IC_INPUT_CHANNELS 2
#define IC_OUTPUT_CHANNELS 2 //0:ASR, 1:Comms

#endif
