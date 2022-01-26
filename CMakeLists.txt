cmake_minimum_required(VERSION 3.11)

## Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

project(sw_avona)

## With this hack, x86 builds are work on Linux and Darwin.
if( NOT XCORE )
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_VERSION 0.0.1)
endif()

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

# The path to the xcore_sdk must be set either as the CMake variable XCORE_SDK_PATH or as an
# environment variable XCORE_SDK_PATH
if(NOT DEFINED XCORE_SDK_PATH)
  if(NOT DEFINED ENV{XCORE_SDK_PATH})
    message(FATAL_ERROR "XCORE_SDK_PATH environment variable must be set")
  endif()

  set(XCORE_SDK_PATH $ENV{XCORE_SDK_PATH})
endif()

get_filename_component(XCORE_SDK_PATH ${XCORE_SDK_PATH} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(NOT EXISTS ${XCORE_SDK_PATH})
  message(FATAL_ERROR "xcore_sdk not present at ${XCORE_SDK_PATH}")
endif()

message(STATUS "xcore_sdk in ${XCORE_SDK_PATH}")

## Add libs and apps
add_subdirectory( modules )

if ( ${BUILD_EXAMPLES} )
    add_subdirectory( examples )
endif()

if ( ${BUILD_TESTS} )
  add_subdirectory( test )
endif()
