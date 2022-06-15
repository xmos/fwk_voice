## fetch dependencies that are required to build or test modules and examples
include(FetchContent)

FetchContent_Declare(
    xs3_math
    GIT_REPOSITORY https://github.com/xmos/lib_xs3_math.git
    GIT_TAG        918aa48b1f6cb284c5db31af1f77592f650e4324
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/lib_xs3_math
)
FetchContent_Populate(xs3_math)

FetchContent_Declare(
    xscope_fileio
    GIT_REPOSITORY https://github.com/xmos/xscope_fileio.git
    GIT_TAG        86add5101d73d98d4addea9aaeb238072e461b63
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/xscope_fileio
)
FetchContent_Populate(xscope_fileio)

FetchContent_Declare(
    lib_nn
    GIT_REPOSITORY https://github.com/xmos/lib_nn.git
    GIT_TAG        1a0fce0f20b4bbb95dcc72c381d4e88c7b08c9e5
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/lib_nn
)
FetchContent_Populate(lib_nn)

FetchContent_Declare(
    tflite_micro
    GIT_REPOSITORY https://github.com/xmos/lib_tflite_micro.git
    GIT_TAG        c85e3be656bd4e250df7859892afede7aaca81d0
    GIT_SHALLOW    FALSE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/lib_tflite_micro
)
FetchContent_Populate(tflite_micro)
