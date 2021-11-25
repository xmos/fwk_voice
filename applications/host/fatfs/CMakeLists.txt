cmake_minimum_required(VERSION 3.20)

project(fatfs_mkimage LANGUAGES C)
set(TARGET_NAME fatfs_mkimage)

# Disable in-source build.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

# Tell cmake where to install things
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE PATH "" FORCE)

## Import XMOS configurations
if(DEFINED XCORE_SDK_PATH)
    include("${XCORE_SDK_PATH}/modules/rtos/sw_services/fatfs/host/CMakeLists.txt")
else()
    include("$ENV{XCORE_SDK_PATH}/modules/rtos/sw_services/fatfs/host/CMakeLists.txt")
endif()