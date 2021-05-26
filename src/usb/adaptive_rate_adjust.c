// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#define DEBUG_UNIT ADAPTIVE_USB

#include <stdbool.h>
#include <xcore/port.h>
#include <rtos_printf.h>
#include <dsp.h>
#include <xscope.h>

#include "app_pll_ctrl.h"

/*
 * Setting this non-zero initializes the appPLL numerator
 * to 0 to allow the PID controller's step response to be
 * viewed via xscope.
 */
#define PID_TEST 0

/*
 * 125 microseconds between each SOF at USB high speed.
 */
#define SOF_DELTA_TIME_US 125

/*
 * If the time between SOFs is off by more than
 * this, then reset the controller's error state.
 */
#define SOF_VALID_MAX_ERROR 0.05

/*
 * If the time between SOFs is off by more than
 * this, then assume it came in a little late or early
 * due to the interrupt being delayed or USB bus traffic,
 * and do not update the PID's output, but continue to
 * integrate the error.
 */
#define SOF_ON_TIME_MAX_ERROR 0.003

#define REFERENCE_CLOCK_TICKS_PER_MICROSECOND 100
#define US_TO_TICKS(us) (us * REFERENCE_CLOCK_TICKS_PER_MICROSECOND)

#define SOF_DELTA_TIME_NOMINAL US_TO_TICKS(SOF_DELTA_TIME_US)
#define SOF_DELTA_TIME_VALID_LOWER ((1.0 - SOF_VALID_MAX_ERROR) * SOF_DELTA_TIME_NOMINAL)
#define SOF_DELTA_TIME_VALID_UPPER ((1.0 + SOF_VALID_MAX_ERROR) * SOF_DELTA_TIME_NOMINAL)
#define SOF_DELTA_TIME_ON_TIME_LOWER ((1.0 - SOF_ON_TIME_MAX_ERROR) * SOF_DELTA_TIME_NOMINAL)
#define SOF_DELTA_TIME_ON_TIME_UPPER ((1.0 + SOF_ON_TIME_MAX_ERROR) * SOF_DELTA_TIME_NOMINAL)

/*
 * The ideal number of appPLL cycles between each SOF that
 * the PID controller is aiming for.
 */
#define DELTA_CYCLES_TARGET (((appconfAUDIO_CLOCK_FREQUENCY / 10) * SOF_DELTA_TIME_US) / 100000)

/*
 * The number of fractional bits in the fixed point
 * numbers used by the PID controller.
 */
#define P 16

#if 1
bool tud_xcore_sof_cb(uint8_t rhport)
{
    static int32_t numerator = PID_TEST ? 0 : Q(P)((APP_PLL_FRAC_NOM & 0x0000FF00) >> 8);
    static int32_t previous_error;
    static int32_t integral;

    static uint32_t last_time;
    static uint16_t last_cycle_count;

    int valid;
    int on_time;

    uint16_t cur_cycle_count;
    uint16_t delta_cycles;
    uint32_t cur_time;
    uint32_t delta_time;

    asm volatile(
            "{gettime %0; getts %1, res[%2]}"
            : "=r"(cur_time), "=r"(cur_cycle_count)
            : "r"(PORT_MCLK_IN)
            : /* no clobbers */
            );

    delta_time = cur_time - last_time;
    last_time = cur_time;
    valid = delta_time >= SOF_DELTA_TIME_VALID_LOWER && delta_time <= SOF_DELTA_TIME_VALID_UPPER;
    on_time = delta_time >= SOF_DELTA_TIME_ON_TIME_LOWER && delta_time <= SOF_DELTA_TIME_ON_TIME_UPPER;

    delta_cycles = cur_cycle_count - last_cycle_count;
    last_cycle_count = cur_cycle_count;

    if (valid) {

        int32_t error = DELTA_CYCLES_TARGET - delta_cycles;

        /*
         * Always integrate the error, even if this SOF is not on time.
         * Late SOFs are always followed by early SOFs, so the integrated
         * error will even out, even if the current error value is off.
         */
        integral += error;

        /*
         * If the current SOF came in either a little late or early (as
         * measured by the reference clock) then the instantaneous error
         * cannot be trusted, and no adjustment to the PLL frequency should
         * be made.
         */
        if (on_time) {
            const int32_t kp = Q(P)(0.1);
            const int32_t ki = Q(P)(0.0001);
            const int32_t kd = Q(P)(0);

            int32_t output;
            int32_t numerator_int;
            const int32_t proportional = error;
            const int32_t derivative = error - previous_error;
            previous_error = error;

            output = dsp_math_multiply(kp, proportional, 0) +
                     dsp_math_multiply(ki, integral, 0) +
                     dsp_math_multiply(kd, derivative, 0);

            numerator += output;
            if (numerator > Q(P)(255)) {
                numerator = Q(P)(255);
            } else if (numerator < 0) {
                numerator = 0;
            }

            numerator_int = (numerator + Q(P)(0.5)) >> P;

            output += Q(P)(0.5);
            output >>= P;

            rtos_printf("%u (%d, %d, %d) -> %d -> %d\n", delta_cycles,
                        proportional, integral, derivative,
                        output, numerator_int);

            app_pll_set_numerator(numerator_int);
            xscope_int(PLL_FREQ, numerator_int);
        } else {
            rtos_printf("no adjustment from PID\n");
        }

    } else {
        /*
         * The SOF arrived far too late or early. Reset the error state,
         * but leave the current PLL frequency alone. This might be due
         * to a USB reset for example.
         */
        previous_error = 0;
        integral = 0;
        rtos_printf("reset PID\n");
    }

    /* False tells TinyUSB to not send the SOF event to the stack */
    return false;
}
#endif
