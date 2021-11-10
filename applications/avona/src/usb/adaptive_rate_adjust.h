/*
 * adaptive_rate_adjust.h
 *
 *  Created on: Sep 15, 2021
 *      Author: mbruno
 */

#ifndef ADAPTIVE_RATE_ADJUST_H_
#define ADAPTIVE_RATE_ADJUST_H_

#include <xcore/channel.h>
#include <xcore/clock.h>

void adaptive_rate_adjust_init(chanend_t other_tile_c, xclock_t mclk_clkblk);

#endif /* ADAPTIVE_RATE_ADJUST_H_ */
