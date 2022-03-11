# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from pathlib import Path
import shutil

# One more ../ than necessary - builds in the 'build' folder
MODULE_ROOT = "../../../../modules"
FLAGS = [
    '-std=c99',
    '-L../../../build/modules/lib_ic', 
    '-libavona_module_lib_ic.a'
    ]

INCLUDE_DIRS=[
    f"{MODULE_ROOT}/lib_ic/api/",
    f"{MODULE_ROOT}/lib_ic/src/",

]
SRCS = f"../ic_test.c".split()
ffibuilder = FFI()

# Contains all the C defs visible from Python
ffibuilder.cdef(
"""
    int test_frame(int in);
"""
)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("ic_test_py",  # name of the output C extension
"""
    //#include "ic_api.h"
    int test_frame(int in);
""",
    sources=SRCS,
    libraries=['m'],    # on Unix, link with the math library
    extra_compile_args=FLAGS,
    include_dirs=INCLUDE_DIRS)

if __name__ == "__main__":
    ffibuilder.compile(tmpdir='build', target='ic_test_py.*', verbose=True)
    #Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    shutil.copyfile("build/ic_test_py.dylib", "build/ic_test_py.so")

