cmake_minimum_required(VERSION 3.21)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(fwk_voice)

enable_language(CXX C ASM)

find_package( Python3 COMPONENTS Interpreter )

## Add libs and apps
add_subdirectory( lib_voice )

if(PROJECT_IS_TOP_LEVEL)

    ## Compile flags for C/C++ for all apps (for all platforms)
    list(APPEND   COMPILE_FLAGS        -Os -MMD -g)
    list(APPEND   COMPILE_FLAGS        -Wno-format -Wall )

    message( STATUS "System name ${CMAKE_SYSTEM_NAME}" )

    ## Fetch lib_xcore_math
    set(XCORE_MATH_PATH ${CMAKE_BINARY_DIR}/fwk_voice_deps)
    include(FetchContent)
    FetchContent_Declare(
        xcore_math
        GIT_REPOSITORY https://github.com/xmos/lib_xcore_math.git
        GIT_TAG        develop
        GIT_SHALLOW    FALSE
        SOURCE_DIR     ${XCORE_MATH_PATH}/lib_xcore_math
    )
    FetchContent_Populate(xcore_math)
    add_subdirectory(${XCORE_MATH_PATH}/lib_xcore_math/lib_xcore_math ${XCORE_MATH_PATH}/build)

    ## Platform-specific C/C++ compile flags
    if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
        set(XCORE_TARGET "XCORE-AI-EXPLORER")
    else()
        list(APPEND   COMPILE_FLAGS  -DX86_BUILD=1)
    endif()

    add_compile_options( ${COMPILE_FLAGS} )
    add_subdirectory( examples )

    set( CONFIG_XSCOPE_PATH ${CMAKE_SOURCE_DIR}/test/shared/file_utils)
    add_subdirectory( test )
endif()
