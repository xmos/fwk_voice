// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_PLL_CTRL_H_
#define APP_PLL_CTRL_H_

#include "board_init.h" // for AUDIO_CLOCK_FREQUENCY

#if (AUDIO_CLOCK_FREQUENCY != 24576000)
#error PLL values only valid if AUDIO_CLOCK_FREQUENCY == 24576000
#endif

#define APP_PLL_NOM   0x0A01FF04 // 24576000
#define APP_PLL_SLOW  0x0A01FE04 // 24624000 ~0.2% slower
#define APP_PLL_FAST  0x0A020004 // 24528000 ~0.2% faster

void set_app_pll(unsigned pll_ctl_num);

#endif /* APP_PLL_CTRL_H_ */
