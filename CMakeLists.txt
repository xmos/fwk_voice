cmake_minimum_required(VERSION 3.11)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(xmos-sw_avona)

## Set up the build options
include(etc/build_options.cmake)

## Fetch the Unity repo as a dependency
include(etc/fetch_unity.cmake)

find_package( Python3 COMPONENTS Interpreter )

## Compile flags for C/C++ for all libs and apps (for all platforms)
list(APPEND   COMPILE_FLAGS        -Os -g -MMD   )
list(APPEND   COMPILE_FLAGS        -Wno-format -Wall )

## Platform-specific C/C++ compile flags
list(APPEND   COMPILE_FLAGS_XCORE  -march=xs3a )

list(APPEND   COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} )

## CMake configuration stuff

enable_language( C CXX ASM )

set( CMAKE_CXX_STANDARD           11  )
set( CMAKE_CXX_STANDARD_REQUIRED  ON  )
set( CMAKE_CXX_EXTENSIONS         OFF )

add_compile_options( ${COMPILE_FLAGS} )

## Add libs and apps
add_subdirectory( ${CMAKE_SOURCE_DIR}/../lib_xs3_math/lib_xs3_math ${CMAKE_CURRENT_BINARY_DIR}/libs/)
add_subdirectory( modules )


if ( ${BUILD_TESTS} )
  add_subdirectory( test )
endif()