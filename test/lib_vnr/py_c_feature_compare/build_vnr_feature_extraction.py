# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from cffi import FFI
from pathlib import Path
import shutil
from extract_state import extract_pre_defs_vnr
import sys

# One more ../ than necessary - builds in the 'build' folder
MODULE_ROOT = "../../../../modules"
XCORE_MATH = "../../../../build/fwk_voice_deps/lib_xcore_math"

FLAGS = [
    '-std=c99',
    '-fPIC'
    ]

INCLUDE_DIRS=[
    f"{MODULE_ROOT}/lib_vnr/api/common",
    f"{MODULE_ROOT}/lib_vnr/api/features",
    f"{MODULE_ROOT}/lib_vnr/src/features",
    f"{MODULE_ROOT}/lib_vnr/api/inference",
    f"{MODULE_ROOT}/lib_vnr/src/inference/model",
    f"{MODULE_ROOT}/lib_vnr/src/inference",
    f"{XCORE_MATH}/lib_xcore_math/api",
]
SRCS = f"../vnr_test.c".split()
ffibuilder = FFI()

#Extract all defines and state from lib_vnr programatically
predefs = extract_pre_defs_vnr()
predefs = predefs.replace("sizeof(uint64_t)", "8") # Fix uint64_t tensor_arena[TENSOR_ARENA_SIZE_BYTES/sizeof(uint64_t)]; in vnr_ie_state_t
print(predefs)
# Contains all the C defs visible from Python
ffibuilder.cdef(
predefs +
"""
    int test_init(void);
    vnr_feature_state_t test_get_feature_state(void);
    void test_vnr_features(bfp_s32_t *feature_bfp, int32_t *feature_data, const int32_t *new_x_frame);
    double test_vnr_inference(bfp_s32_t *features);
"""
)

# Contains the C source necessary to allow the cdefs to work
ffibuilder.set_source("vnr_test_py",  # name of the output C extension
"""
    #include "vnr_features_api.h"
    #include "vnr_inference_api.h" 
    int test_init(void);
    vnr_feature_state_t test_get_feature_state(void);
    void test_vnr_features(bfp_s32_t *feature_bfp, int32_t *feature_data, const int32_t *new_x_frame);
    double test_vnr_inference(bfp_s32_t *features);
""",
    sources=SRCS,
    library_dirs=[
                '../../../../build/modules/lib_vnr',
                '../../../../build/test/lib_vnr',
                '../../../../build/fwk_voice_deps/build'
                    ],
    libraries=['fwk_voice_module_lib_vnr_inference', 'fwk_voice_module_lib_vnr_features', 'lib_xcore_math', 'm', 'stdc++'],    # on Unix, link with the math library. Linking order is important here for gcc compile on Linux!
    extra_compile_args=FLAGS,
    include_dirs=INCLUDE_DIRS)


if __name__ == "__main__":
    ffibuilder.compile(tmpdir='build', target='vnr_test_py.*', verbose=True)
    #Darwin hack https://stackoverflow.com/questions/2488016/how-to-make-python-load-dylib-on-osx
    if sys.platform == "darwin":
        shutil.copyfile("build/vnr_test_py.dylib", "build/vnr_test_py.so")
