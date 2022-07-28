// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef FDAF_DEFINES_H
#define FDAF_DEFINES_H

#include <bfp_math.h>
#include <string.h>

#define FDAF_FRAME_ADVANCE 240
#define FDAF_PROC_FRAME_LENGTH 512
#define FDAF_F_BIN_COUNT (FDAF_PROC_FRAME_LENGTH / 2) + 1
#define FDAF_FRAME_OVERLAP (FDAF_FRAME_LENGTH - (2 * FDAF_FRAME_ADVANCE))
#define FDAF_ZEROVAL_EXP -1024
#define FDAF_ZEROVAL_HR 31
#define FDAF_UNUSED_TAPS_PER_PHASE 16

#endif
