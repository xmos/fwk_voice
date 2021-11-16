cmake_minimum_required(VERSION 3.20)

project(avona_control LANGUAGES C)
set(TARGET_NAME fatfs_mkimage)

# Disable in-source build.
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

# Tell cmake where to install things
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE PATH "" FORCE)

## Import XMOS configurations
if(DEFINED XCORE_SDK_PATH)
    set(CMAKE_SOURCE_DIR "${XCORE_SDK_PATH}/modules/rtos/sw_services/fatfs/host/")
else()
    set(CMAKE_SOURCE_DIR "$ENV{XCORE_SDK_PATH}/modules/rtos/sw_services/fatfs/host/")
endif()

set(APP_SOURCES
    "${CMAKE_SOURCE_DIR}/fatfs_mkimage.c"
    "${CMAKE_SOURCE_DIR}/fatfs_ops.c"
    "${CMAKE_SOURCE_DIR}/directory_add.c"
    "${CMAKE_SOURCE_DIR}/diskio.c"
    "${CMAKE_SOURCE_DIR}/ramdisk.c"
    "${CMAKE_SOURCE_DIR}/ffsystem.c"
    "${CMAKE_SOURCE_DIR}/argtable/argtable3.c"
    "${CMAKE_SOURCE_DIR}/../thirdparty/src/ff.c"
)

set(APP_INCLUDES
    "${CMAKE_SOURCE_DIR}/."
    "${CMAKE_SOURCE_DIR}/../thirdparty/api/"
)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${APP_INCLUDES})

if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_C_COMPILER_ID STREQUAL "AppleClang"))
    message(STATUS "Configuring for Clang")
    target_compile_options(${TARGET_NAME} PRIVATE -O2 -Wall)
    target_link_options(${TARGET_NAME} PRIVATE "")
elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Configuring for GCC")
    target_compile_options(${TARGET_NAME} PRIVATE -O2 -Wall)
    target_link_options(${TARGET_NAME} PRIVATE "")
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Configuring for MSVC")
    target_compile_options(${TARGET_NAME} PRIVATE /W3)
    target_link_options(${TARGET_NAME} PRIVATE "")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
else ()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_C_COMPILER_ID}")
endif()