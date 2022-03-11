# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from pathlib import Path
import shutil

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

# Contains all the C defs visible from Python
ffibuilder.cdef(
"""

    int test_frame(int in);
    //ic_state_t get_state(void);
"""
)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("ic_test_py",  # name of the output C extension
"""
    #include "ic_api.h"
    int test_frame(int in);

    typedef struct {
        int a[30];
    }grr_t;

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

