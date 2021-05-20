// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_PLL_CTRL_H_
#define APP_PLL_CTRL_H_

#include "board_init.h" // for AUDIO_CLOCK_FREQUENCY

#ifndef APP_PLL_NUDGE_METHOD
#define APP_PLL_NUDGE_METHOD 2
#endif

#if (AUDIO_CLOCK_FREQUENCY != 24576000)
#error PLL values only valid if AUDIO_CLOCK_FREQUENCY == 24576000
#endif

#define APP_PLL_CTL_VAL 0x0A019803 // Valid for all fractional values

#if APP_PLL_NUDGE_METHOD == 1
#define APP_PLL_FRAC_NOM   0x80000204 // 24576000
#define APP_PLL_FRAC_SLOW  0x80000209 // 24558000 ~0.07% slower
#define APP_PLL_FRAC_FAST  0x80000809 // 24594000 ~0.07% faster
#elif APP_PLL_NUDGE_METHOD == 2
#define APP_PLL_FRAC_NOM   0x800095F9 // 24576000
#define APP_PLL_FRAC_SLOW  0x80004AF9 // 24558000 ~0.07% slower
#define APP_PLL_FRAC_FAST  0x8000E0F9 // 24594000 ~0.07% faster
#endif

#define APP_PLL_NUDGE_FASTER  1
#define APP_PLL_NUDGE_SLOWER -1

void app_pll_nudge(int nudge);

void app_pll_init(void);

#endif /* APP_PLL_CTRL_H_ */
