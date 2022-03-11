# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from pathlib import Path
import shutil
from extract_state import extract_pre_defs

# One more ../ than necessary - builds in the 'build' folder
MODULE_ROOT = "../../../../modules"
XS3_MATH = "../../../../xcore_sdk/modules/lib_xs3_math/lib_xs3_math"

FLAGS = [
    '-std=c99',
    ]

INCLUDE_DIRS=[
    f"{MODULE_ROOT}/lib_ic/api/",
    f"{MODULE_ROOT}/lib_ic/src/",
    f"{XS3_MATH}/lib_xs3_math/api/",
]
SRCS = f"../ic_test.c".split()
ffibuilder = FFI()

#Extract all defines and state from lib_ic programatically
predefs = extract_pre_defs()
# Contains all the C defs visible from Python
ffibuilder.cdef(
predefs+
"""
    void test_init(void);
    ic_state_t test_get_state(void);
    void test_filter(int32_t y_data[IC_FRAME_ADVANCE], int32_t x_data[IC_FRAME_ADVANCE], int32_t output[IC_FRAME_ADVANCE]);
    void test_adapt(uint8_t vad, int32_t output[IC_FRAME_ADVANCE]);
""".replace("IC_FRAME_ADVANCE", "240")
)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("ic_test_py",  # name of the output C extension
"""
    #include "ic_api.h"
    void test_init(void);
    ic_state_t test_get_state(void);
    void test_filter(int32_t y_data[IC_FRAME_ADVANCE], int32_t x_data[IC_FRAME_ADVANCE], int32_t output[IC_FRAME_ADVANCE]);
    void test_adapt(uint8_t vad, int32_t output[IC_FRAME_ADVANCE]);
""",
    sources=SRCS,
    library_dirs=[
                '../../../../build/modules/lib_ic',
                '../../../../build/modules/lib_aec',
                '../../../../build/examples/bare-metal/shared_src/external_deps/lib_xs3_math'
                    ],
    libraries=['m', 'avona_module_lib_ic', 'avona_module_lib_aec', 'avona_deps_lib_xs3_math'],    # on Unix, link with the math library
    extra_compile_args=FLAGS,
    include_dirs=INCLUDE_DIRS)

if __name__ == "__main__":
    ffibuilder.compile(tmpdir='build', target='ic_test_py.*', verbose=True)
    #Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    shutil.copyfile("build/ic_test_py.dylib", "build/ic_test_py.so")

