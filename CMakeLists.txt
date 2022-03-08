cmake_minimum_required(VERSION 3.20)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(sw_avona)

enable_language(CXX C ASM)

find_package( Python3 COMPONENTS Interpreter )

## Compile flags for C/C++ for all libs and apps (for all platforms)
list(APPEND   COMPILE_FLAGS        -Os -g -MMD )
list(APPEND   COMPILE_FLAGS        -Wno-format -Wall )

## CMake configuration stuff

set( CMAKE_CXX_STANDARD           11  )
set( CMAKE_CXX_STANDARD_REQUIRED  ON  )
set( CMAKE_CXX_EXTENSIONS         OFF )


message( STATUS "System name ${CMAKE_SYSTEM_NAME}" )

## Platform-specific C/C++ compile flags
if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
    set(XCORE_TARGET "XCORE-AI-EXPLORER")
else()
    list(APPEND   COMPILE_FLAGS  -DX86_BUILD=1)
endif()

add_compile_options( ${COMPILE_FLAGS} )

## Add libs and apps
add_subdirectory( modules )

add_subdirectory( etc )

add_subdirectory( examples )

if ( ${AVONA_BUILD_TESTS} )
    add_subdirectory( test )
endif()
