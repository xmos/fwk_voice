cmake_minimum_required(VERSION 3.11)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(sw_avona)

## Set up the build options
include(etc/build_options.cmake)

find_package( Python3 COMPONENTS Interpreter )

## Compile flags for C/C++ for all libs and apps (for all platforms)
list(APPEND   COMPILE_FLAGS        -Os -g -MMD   )
list(APPEND   COMPILE_FLAGS        -Wno-format -Wall )

## Platform-specific C/C++ compile flags
list( APPEND   COMPILE_FLAGS_XCORE  -march=xs3a )
list( APPEND   COMPILE_FLAGS_Linux  -DX86_BUILD=1 )

list(APPEND   COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} )

## CMake configuration stuff

message( STATUS "System name here ${CMAKE_SYSTEM_NAME}" )
if( XCORE )
    enable_language( C CXX XC ASM )
else()
    enable_language( C CXX ASM )
endif()

set( CMAKE_CXX_STANDARD           11  )
set( CMAKE_CXX_STANDARD_REQUIRED  ON  )
set( CMAKE_CXX_EXTENSIONS         OFF )

add_compile_options( ${COMPILE_FLAGS} )

## Get build dependencies
#file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/deps)
#include(${CMAKE_SOURCE_DIR}/etc/fetch_deps.cmake)

## Add libs and apps
add_subdirectory( modules )

if ( ${BUILD_TESTS} )
  add_subdirectory( test )
endif()
