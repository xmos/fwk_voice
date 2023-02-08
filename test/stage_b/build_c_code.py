# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from pathlib import Path
import shutil
from extract_state import extract_pre_defs
import sys

# One more ../ than necessary - builds in the 'build' folder
MODULE_ROOT = "../../../modules"
XCORE_MATH = "../../../build/fwk_voice_deps/lib_xcore_math/"

FLAGS = [
    '-std=c99',
    '-fPIC'
    ]

INCLUDE_DIRS=[
    f"{MODULE_ROOT}/lib_ic/api/",
    f"{MODULE_ROOT}/lib_ic/src/",
    f"{MODULE_ROOT}/lib_vnr/api/common",
    f"{MODULE_ROOT}/lib_vnr/api/features",
    f"{MODULE_ROOT}/lib_vnr/src/features",
    f"{MODULE_ROOT}/lib_vnr/api/inference",
    f"{MODULE_ROOT}/lib_vnr/src/inference/model",
    f"{MODULE_ROOT}/lib_vnr/src/inference",
    f"{XCORE_MATH}/lib_xcore_math/api",
]
SRCS = f"../ic_vnr_test.c".split()
ffibuilder = FFI()

#Extract all defines and state from lib_ic programatically as CFFI doesn't do that yet
predefs = extract_pre_defs()
predefs += """
    int test_init(void);
    ic_state_t test_get_ic_state(void);
    void test_filter(int32_t y_data[IC_FRAME_ADVANCE], int32_t x_data[IC_FRAME_ADVANCE], int32_t output[IC_FRAME_ADVANCE]);
    void test_adapt(float_s32_t vnr);
    float_s32_t test_vnr();
    void test_control_system(double vnr_fl, int32_t ad_config, double fast_ratio);
"""
#Bit of a faff having to do this. The preproc doesn't handle mul/div or enums or even #define so do it here manually
replacements = [["bfp_flags_e", "int"],
                ["IC_FRAME_ADVANCE", "240"],
                ["VNR_FRAME_ADVANCE", "240"],
                ["512 / 2", "256"],
                ["2 * 240", "480"],
                ["1*10", "10"],
                ["10*1", "10"],
                ["sizeof(uint64_t)", "8"]]

for replacement in replacements:
    predefs = predefs.replace(*replacement)

print(predefs)

# Contains all the C defs visible from Python
ffibuilder.cdef(predefs)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("ic_vnr_test_py",  # name of the output C extension
"""
    #include "ic_api.h"
    #include "ic_low_level.h"
    int test_init(void);
    ic_state_t test_get_ic_state(void);
    void test_filter(int32_t y_data[IC_FRAME_ADVANCE], int32_t x_data[IC_FRAME_ADVANCE], int32_t output[IC_FRAME_ADVANCE]);
    void test_adapt(float_s32_t vnr);
    float_s32_t test_vnr();
    void test_control_system(double vnr_fl, int32_t ad_config, double fast_ratio);
""",
    sources=SRCS,
    library_dirs=[
                '../../../build/modules/lib_ic',
                '../../../build/modules/lib_aec',
                '../../../build/modules/lib_vnr',
                '../../../build/fwk_voice_deps/build'
                    ],
    libraries=['fwk_voice_module_lib_ic', 'fwk_voice_module_lib_aec', 'fwk_voice_module_lib_vnr_inference', 'fwk_voice_module_lib_vnr_features', 'lib_xcore_math', 'm', 'stdc++'],    # on Unix, link with the math library. Linking order is important here for gcc compile on Linux
    extra_compile_args=FLAGS,
    include_dirs=INCLUDE_DIRS)

if __name__ == "__main__":
    ffibuilder.compile(tmpdir='build', target='ic_vnr_test_py.*', verbose=True)
    #Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    if sys.platform == "darwin":
        shutil.copyfile("build/ic_vnr_test_py.dylib", "build/ic_vnr_test_py.so")

