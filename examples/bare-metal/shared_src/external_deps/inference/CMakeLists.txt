add_library(xcore_sdk_inferencing_lib_tflite_micro INTERFACE)
SET(TOP_DIR ${CMAKE_BINARY_DIR}/fwk_voice_deps/lib_tflite_micro)
include(${TOP_DIR}/cmakefiles/xtflm.cmake)

target_sources(xcore_sdk_inferencing_lib_tflite_micro
    INTERFACE
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
    target_sources(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            ${LIB_NN_SOURCES_XCORE_XS3A}
            ${TFLIB_SOURCES_ASM}
    )
    target_compile_options(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            -Wno-xcore-fptrgroup
            -mcmodel=large
    )
else()
    target_sources(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            ${TOP_DIR}/lib_tflite_micro/submodules/flatbuffers/src/util.cpp #link error on x86
    )
    set(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "C++ Compiler Base Flags" FORCE)
    target_compile_definitions(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
           NN_USE_REF
    )
endif()

target_include_directories(xcore_sdk_inferencing_lib_tflite_micro
    INTERFACE
        src
        ${ALL_INCLUDES})

target_compile_options(xcore_sdk_inferencing_lib_tflite_micro
        INTERFACE
            -Os
            -g
    )

target_compile_definitions(xcore_sdk_inferencing_lib_tflite_micro
    INTERFACE
        NO_INTERPRETER
        TF_LITE_STATIC_MEMORY=1
        TF_LITE_STRIP_ERROR_STRINGS)

add_library(sdk::inferencing::lib_tflite_micro ALIAS xcore_sdk_inferencing_lib_tflite_micro)
