# Copyright 2019-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from cffi import FFI
from pathlib import Path
import shutil

# One more ../ than necessary - builds in the 'build' folder
APP = "../../../app_xk_xvf3510_l71"
SRC = f"{APP}/xua_lite/rate_control/rate_controller.c"
FLAGS = ['-D TESTING', '-D REF_TO_AP_RATIO=3', '-std=c99']
INCLUDE_DIRS=[
    f"{APP}/xua_lite/",
    f"{APP}/xua_lite/rate_control",
    f"{APP}/config",
    "../dummy_includes"
]

ffibuilder = FFI()

# Contains all the C defs visible from Python
ffibuilder.cdef(
"""
    typedef int32_t xua_lite_fixed_point_t;
    typedef struct pid_state_t {
      xua_lite_fixed_point_t fifo_level_filtered_old;
      xua_lite_fixed_point_t fifo_level_accum;
      xua_lite_fixed_point_t p_term;
      xua_lite_fixed_point_t i_term;
      xua_lite_fixed_point_t d_term;
    } pid_state_t;
    xua_lite_fixed_point_t do_rate_control(int fill_level,
                                           int target_level,
                                           pid_state_t *pid_state);
    void do_clock_nudge_pdm(xua_lite_fixed_point_t controller_out,
                            int *clock_nudge);
    xua_lite_fixed_point_t do_rate_control_custom(
        int fill_level,
        int target_level,
        pid_state_t *pid_state,
        const int64_t pid_control_p_term_coeff,
        const int64_t pid_control_i_term_coeff,
        const int64_t pid_control_d_term_coeff
    );
    // PID parameter #define getters
    int xua_lite_fixed_point_one();
    int pid_rate_multiplier();
    int pid_calc_overhead_bits();
"""
)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("rate_controller_py",  # name of the output C extension
"""
    #include "rate_controller.h"
    int xua_lite_fixed_point_one() {
        return XUA_LIGHT_FIXED_POINT_ONE;
    }
    int pid_rate_multiplier() {
        return PID_RATE_MULTIPLIER;
    }
    int pid_calc_overhead_bits() {
        return PID_CALC_OVERHEAD_BITS;
    }
""",
    sources=[SRC],
    libraries=['m'],    # on Unix, link with the math library
    extra_compile_args=FLAGS,
    include_dirs=INCLUDE_DIRS)

if __name__ == "__main__":
    ffibuilder.compile(tmpdir='build', target='rate_controller_py.*', verbose=True)
