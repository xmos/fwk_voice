cmake_minimum_required(VERSION 3.21)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(fwk_voice)

enable_language(CXX C ASM)

## Add libs and apps
add_subdirectory( modules )

if(PROJECT_IS_TOP_LEVEL)
    find_package( Python3 COMPONENTS Interpreter )

    ## Compile flags for C/C++ for all apps (for all platforms)
    list(APPEND   COMPILE_FLAGS        -Os -MMD -g)
    list(APPEND   COMPILE_FLAGS        -Wno-format -Wall )

    message( STATUS "System name ${CMAKE_SYSTEM_NAME}" )

    ## Platform-specific C/C++ compile flags
    if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
        set(XCORE_TARGET "XCORE-AI-EXPLORER")
    else()
        list(APPEND   COMPILE_FLAGS  -DX86_BUILD=1)
    endif()

    add_compile_options( ${COMPILE_FLAGS} )
    add_subdirectory( examples )

    add_subdirectory( test )
endif()
