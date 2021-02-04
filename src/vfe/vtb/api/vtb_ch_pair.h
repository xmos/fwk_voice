// Copyright (c) 2018-2019, XMOS Ltd, All rights reserved
#ifndef VTB_CH_PAIR_H_
#define VTB_CH_PAIR_H_

#include "dsp.h"

/**
 * Struct containing the sample data of two channels.
 * An array of this struct can be used to hold a frame of samples.
 */
typedef dsp_ch_pair_t vtb_ch_pair_t;

/**
 * Struct containing the sample data of two channels.
 * An array of this struct can be used to hold a frame of samples.
 */
typedef dsp_complex_short_t vtb_complex_short_t;


/**
 * Struct containing the floating poing sample data of two channels.
 * An array of this struct can be used to hold a frame of samples.
 */
typedef dsp_ch_pair_fp vtb_ch_pair_fp;

#endif /* VTB_CH_PAIR_H_ */
