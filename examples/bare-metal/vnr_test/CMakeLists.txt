add_library(tflite_test_lib STATIC)
SET(TOP_DIR ${CMAKE_BINARY_DIR}/fwk_voice_deps/lib_tflite_micro)
include(${TOP_DIR}/cmakefiles/xtflm.cmake)

file(GLOB_RECURSE TEST_SOURCES src/*.cpp)
target_sources(tflite_test_lib
    PRIVATE 
        ${TEST_SOURCES}
        ${TFLM_KERNEL_SOURCES}
        ${TFLITE_SOURCES}
        ${NN_SOURCES}
        ${XTFLIB_KERNEL_SOURCES}
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    file(GLOB_RECURSE LIB_NN_SOURCES_XCORE_XS3A ${TOP_DIR}/../lib_nn/lib_nn/src/asm/*.S)
    set_source_files_properties(LIB_NN_SOURCES_XCORE_XS3A PROPERTIES LANGUAGE ASM)
    file(GLOB_RECURSE TFLIB_SOURCES_ASM ${TOP_DIR}/lib_tflite_micro/src/*.S)
    set_source_files_properties(TFLIB_SOURCES_ASM PROPERTIES LANGUAGE ASM)
    target_sources(tflite_test_lib
        PRIVATE
            ${LIB_NN_SOURCES_XCORE_XS3A}
            ${TFLIB_SOURCES_ASM}
    )
endif()

target_include_directories(tflite_test_lib
    PUBLIC
        src
        ${ALL_INCLUDES})

target_compile_options(tflite_test_lib
        PRIVATE
            -Os
            -g
            -mcmodel=large
            -Wno-xcore-fptrgroup
    )

target_compile_definitions(tflite_test_lib
    PRIVATE
        NO_INTERPRETER
        TF_LITE_STATIC_MEMORY=1
        TF_LITE_STRIP_ERROR_STRINGS
        __xtflm_conf_h_exists__=1)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)
        target_compile_definitions(tflite_test_lib
            PRIVATE
                NN_USE_REF)
endif()


##############################

add_executable(tflite_test_app)

target_sources(tflite_test_app 
    PRIVATE
        src/main.c
        )
target_include_directories(tflite_test_app
    PRIVATE
        src)

target_link_libraries(tflite_test_app
    PUBLIC
        tflite_test_lib)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
target_link_options(tflite_test_app
        PRIVATE
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
target_link_libraries(tflite_test_app
        PRIVATE m)
endif()
