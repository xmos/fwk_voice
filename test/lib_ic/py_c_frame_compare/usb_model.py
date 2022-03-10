# Copyright 2019-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import matplotlib.pyplot as plt
import numpy as np

from build import rate_controller_py
from rate_controller_py import ffi
import rate_controller_py.lib as rate_controller_lib


class PID:
    def __init__(self, target_level):
        self.target_level = target_level
        # PID state is zero initialised
        self.pid_state = ffi.new("struct pid_state_t *")
        self.frac_clock_nudge = 0


    def do_rate_control(self, out_fifo_level):
        co = rate_controller_lib.do_rate_control(out_fifo_level, self.target_level, self.pid_state)

        clock_nudge = ffi.new("int *")
        rate_controller_lib.do_clock_nudge_pdm(co, clock_nudge)

        return co, clock_nudge[0]


def model_usb(num_sofs,
              init_fifo_level=0,
              fifo_target_level=48,
              sof_rate=1000,
              host_ticks=48000,
              num_usb_chans=4,
              device_ticks_low=47986,
              device_ticks_nom=48000,
              device_ticks_high=48014):

    out_fifo_level = init_fifo_level

    fifo_trace = np.zeros(num_sofs, dtype=np.int)
    fifo_trace_filtered = np.zeros(num_sofs)
    co_trace = np.zeros(num_sofs)
    p_trace = np.zeros(num_sofs)
    i_trace = np.zeros(num_sofs)
    d_trace = np.zeros(num_sofs)
    consecutive_nudges = np.zeros(num_sofs, dtype=np.int)

    pid = PID(fifo_target_level)

    device_ticks = device_ticks_nom

    fp_one = rate_controller_lib.xua_lite_fixed_point_one()

    nudge_count = 0
    for i in range(num_sofs):
        out_fifo_level += host_ticks / sof_rate
        out_fifo_level -= device_ticks / sof_rate

        out_fifo_level_model = int(out_fifo_level) * num_usb_chans

        co, clock_nudge = pid.do_rate_control(out_fifo_level_model)

        if clock_nudge > 0:
            device_ticks = device_ticks_high
        elif clock_nudge < 0:
            device_ticks = device_ticks_low
        else:
            device_ticks = device_ticks_nom

        fifo_trace[i] = out_fifo_level_model
        co_trace[i] = float(co) / fp_one
        p_trace[i] = pid.pid_state.p_term / fp_one
        i_trace[i] = pid.pid_state.i_term / fp_one
        d_trace[i] = pid.pid_state.d_term / fp_one

        fifo_trace_filtered[i] = pid.pid_state.fifo_level_filtered_old / fp_one

        nudge_count = nudge_count + 1 if clock_nudge != 0 else 0
        consecutive_nudges[i] = nudge_count

    #plt.plot(p_trace)
    #plt.title("P")
    #plt.figure()
    #plt.plot(i_trace)
    #plt.title("I")
    #plt.figure()
    #plt.plot(d_trace)
    #plt.title("D")
    #plt.figure()
    #plt.plot(co_trace)
    #plt.title("Controller Out")
    #plt.figure()
    #plt.plot(fifo_trace)
    #plt.title("FIFO Out")
    #plt.show()

    #plt.plot(fifo_trace_filtered)
    #plt.title("FIFO Filtered")
    #plt.show()
    #plt.figure()

    #plt.plot(consecutive_nudges)
    #plt.title("Consecutive Nudges")
    #plt.figure()

    return consecutive_nudges, fifo_trace


if __name__ == "__main__":
    np.random.seed(10)
    consecutive_nudges, fifo_trace = model_usb(1000 * 20, init_fifo_level=0, fifo_target_level=0, host_ticks=47999)

    plt.plot(fifo_trace)
    plt.show()

