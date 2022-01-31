cmake_minimum_required(VERSION 3.11)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

# The path to the xcore_sdk must be set as the environment variable XCORE_SDK_PATH
if(NOT DEFINED ENV{XCORE_SDK_PATH})
  message(FATAL_ERROR "XCORE_SDK_PATH environment variable must be set")
endif()

if(NOT BUILD_NATIVE)
  set(XCORE ON CACHE BOOL "Building for xCore")
  set(XCORE_TARGET "XCORE-AI-EXPLORER" CACHE STRING "xCore hardware target")
  set(BOARD "XCORE-AI-EXPLORER")
  include($ENV{XCORE_SDK_PATH}/tools/cmake_utils/xmos_platform.cmake)

  # Hack: xcore_sdk sets system name as XMOS but we want XCORE for now
  set(CMAKE_SYSTEM_NAME XCORE)
else()
  set(MODULES_DIR $ENV{XCORE_SDK_PATH}/modules)

  # Hack: make x86 builds work on Linux and Darwin
  set(CMAKE_SYSTEM_NAME Linux)
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

enable_language( C CXX ASM )

set( CMAKE_CXX_STANDARD           11  )
set( CMAKE_CXX_STANDARD_REQUIRED  ON  )
set( CMAKE_CXX_EXTENSIONS         OFF )

add_compile_options( ${COMPILE_FLAGS} )

## Add libs and apps
add_subdirectory( modules )

if ( ${BUILD_EXAMPLES} )
    add_subdirectory( examples )
endif()

if ( ${BUILD_TESTS} )
  add_subdirectory( test )
endif()
