## fetch dependencies
include(FetchContent)

FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.5.2
    GIT_SHALLOW    TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/avona_deps/Unity
)
FetchContent_Populate(unity)

FetchContent_Declare(
    xs3_math
    GIT_REPOSITORY https://github.com/xmos/lib_xs3_math.git
    GIT_TAG        b440211755185d97dac8ea72f83f704f8025220f
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/lib_xs3_math
)
FetchContent_Populate(xs3_math)

FetchContent_Declare(
    lib_dsp
    GIT_REPOSITORY https://github.com/xmos/lib_dsp.git
    GIT_TAG        v6.0.2 
    GIT_SHALLOW    TRUE
    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/lib_dsp
)
FetchContent_Populate(lib_dsp)

#FetchContent_Declare(
#    xscope_fileio
#    GIT_REPOSITORY git@github.com:xmos/xscope_fileio.git
#    GIT_TAG        24ac08c57f0ed90379e1324057aa87a4b424b1cb
#    GIT_SHALLOW    TRUE
#    SOURCE_DIR     ${CMAKE_BINARY_DIR}/avona_deps/xscope_fileio
#)
#FetchContent_Populate(xscope_fileio)

