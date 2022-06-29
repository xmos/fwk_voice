## fetch dependencies that are required to build or test modules and examples
include(FetchContent)

FetchContent_Declare(
    xs3_math
    GIT_REPOSITORY https://github.com/xmos/lib_xs3_math.git
    GIT_TAG        b440211755185d97dac8ea72f83f704f8025220f
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

