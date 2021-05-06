cmake_minimum_required(VERSION 3.14)

# Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

## Specify configuration
set(USE_XUD_HIL TRUE)
set(USE_WIFI_MANAGER FALSE)
set(USE_DHCPD FALSE)
set(USE_FATFS FALSE)
set(USE_RTOS_USB_DRIVER TRUE)
set(USE_DEVICE_CONTROL TRUE)
set(USE_TINYUSB TRUE)

## Import XMOS configurations
include("$ENV{XMOS_AIOT_SDK_PATH}/tools/cmake_utils/xmos_rtos_platform.cmake")

## Define project
project(sw_xvf3652)
set(PROJECT_VERSION 0.5.0)

## Enable languages for project
enable_language(XC CXX C ASM)

set(PIPELINE_LIB_COMPILER_FLAGS
    "-Os"
    "-report"
    "-fxscope"
    "-mcmodel=large"
    "-Wno-xcore-fptrgroup"
    "${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}.xn"
)

set(PIPELINE_LIB_SOURCES
    "src/vfe_pipeline/audio_pipeline/audio_pipeline.c"
    "src/vfe_pipeline/vfe_pipeline.c"
)

set(PIPELINE_LIB_INCLUDES
    "src"
    "src/vfe_pipeline/vfe"
    "src/vfe_pipeline/vfe/agc/api"
    "src/vfe_pipeline/vfe/ic/api"
    "src/vfe_pipeline/vfe/ns/api"
    "src/vfe_pipeline/vfe/vad/api"
    "src/vfe_pipeline/vfe/vtb/api"
)

add_library(lib_vfe_pipeline STATIC)

set_target_properties(lib_vfe_pipeline PROPERTIES OUTPUT_NAME vfe_pipeline)
set_target_properties(lib_vfe_pipeline PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/vfe_pipeline")

target_include_directories(lib_vfe_pipeline PRIVATE
    ${PIPELINE_LIB_INCLUDES}
    ${XMOS_RTOS_PLATFORM_INCLUDES}
)

target_sources(lib_vfe_pipeline PRIVATE
    ${PIPELINE_LIB_SOURCES}
)

target_compile_options(lib_vfe_pipeline PRIVATE ${PIPELINE_LIB_COMPILER_FLAGS})
target_link_options(lib_vfe_pipeline PRIVATE ${PIPELINE_LIB_COMPILER_FLAGS})


set(APP_COMPILER_FLAGS
    "-Os"
    "-g"
    "-report"
    "-fxscope"
    "-mcmodel=large"
    "-Wno-xcore-fptrgroup"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.xscope"
    "${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}.xn"
)

set(APP_SOURCES
    "src/main.c"
    "src/board_init.c"
    "src/dac_init.c"
    "src/usb/usb_audio.c"
    "src/usb/usb_descriptors.c"
    "src/usb/usb_device_control.c"
    "src/ww_model_runner/ww_model_runner.c"
)

set(APP_INCLUDES
    "src"
    "src/usb"
    "src/ww_model_runner"
)

add_compile_definitions(
    DEBUG_PRINT_ENABLE=1
    PLATFORM_USES_TILE_0=1
    PLATFORM_USES_TILE_1=1
    XUD_CORE_CLOCK=700
    WW=0
)

add_executable(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} "${CMAKE_SOURCE_DIR}/src/vfe_pipeline/vfe/libvfe.a")
target_link_libraries(${TARGET_NAME} lib_vfe_pipeline)

target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES} ${XMOS_RTOS_PLATFORM_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${APP_INCLUDES} ${XMOS_RTOS_PLATFORM_INCLUDES})

target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
