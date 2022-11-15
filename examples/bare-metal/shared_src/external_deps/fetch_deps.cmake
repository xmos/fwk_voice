## fetch dependencies that are required to build or test modules and examples
include(FetchContent)

FetchContent_Declare(
    xcore_math
    GIT_REPOSITORY https://github.com/xmos/lib_xcore_math.git
    GIT_TAG        v2.1.1
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/fwk_voice_deps/lib_xcore_math
)
FetchContent_Populate(xcore_math)

FetchContent_Declare(
    xscope_fileio
    GIT_REPOSITORY https://github.com/xmos/xscope_fileio.git
    GIT_TAG        2ad04971103f8ca4558d1d2fc903c2a6047b95ba
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/fwk_voice_deps/xscope_fileio
)
FetchContent_Populate(xscope_fileio)

FetchContent_Declare(
    lib_nn
    GIT_REPOSITORY https://github.com/xmos/lib_nn.git
    GIT_TAG        f85b4804ea5f52f5fa4ca4b709a787ac62a8c526
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/fwk_voice_deps/lib_nn
)
FetchContent_Populate(lib_nn)

FetchContent_Declare(
    tflite_micro
    GIT_REPOSITORY https://github.com/xmos/lib_tflite_micro.git
    GIT_TAG        c59ee18f013f159abaa10ef0faf70b18f80f0315
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/fwk_voice_deps/lib_tflite_micro
    PATCH_COMMAND  patch -d lib_tflite_micro/submodules/tflite-micro/ -p0 -i ../../../patches/tflite-micro.patch
)
FetchContent_Populate(tflite_micro)
