cmake_minimum_required(VERSION 3.14)

# Disable in-source build.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source build is not allowed! Please specify a build folder.\n\tex:cmake -B build")
endif()

## Specify configuration
set(USE_XUD_HIL TRUE)
set(USE_FATFS TRUE)
set(USE_RTOS_USB_DRIVER TRUE)
set(USE_DEVICE_CONTROL TRUE)
set(USE_TINYUSB TRUE)

## Import XMOS configurations
include("$ENV{XCORE_SDK_PATH}/tools/cmake_utils/xmos_rtos_platform.cmake")

## Define project
project(sw_avona)

## Enable languages for project
enable_language(CXX C ASM)

set(VFE_LIB_INCLUDES
    "src/vfe_pipeline/vfe"
    "src/vfe_pipeline/vfe/agc/api"
    "src/vfe_pipeline/vfe/ic/api"
    "src/vfe_pipeline/vfe/ns/api"
    "src/vfe_pipeline/vfe/vad/api"
    "src/vfe_pipeline/vfe/vtb/api"
)

set(APP_COMPILER_FLAGS
    "-Os"
    "-g"
    "-report"
    "-fxscope"
    "-mcmodel=large"
    "-Wno-xcore-fptrgroup"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config.xscope"
    "${CMAKE_CURRENT_SOURCE_DIR}/${BOARD}.xn"
    ${APP_CONF_DEFINES}
)

set(APP_SOURCES
    "src/main.c"

    "src/platform/driver_instances.c"
    "src/platform/platform_init.c"
    "src/platform/platform_start.c"
    "src/platform/app_pll_ctrl.c"
    "src/platform/aic3204/aic3204.c"
    "src/platform/dac3101/dac3101.c"

    "src/app_control/app_control.c"
    "src/app_control/ap_control_handlers.c"
    "src/app_control/aec_control_handlers.c"
    "src/app_control/stage1_control_handlers.c"
    "src/app_control/stage2_control_handlers.c"

    "src/vfe_pipeline/audio_pipeline/audio_pipeline.c"
    "src/vfe_pipeline/vfe_pipeline.c"
    "src/vfe_pipeline/burn_cycles.S"

    "src/usb/usb_audio.c"
    "src/usb/usb_descriptors.c"
    "src/usb/adaptive_rate_adjust.c"

    "src/spi/spi_interface.c"

    "src/ww_model_runner/ww_model_runner.c"

    "src/gpio_test/gpio_test.c"
    ${XMOS_RTOS_PLATFORM_SOURCES}
)

set_source_files_properties(src/usb/adaptive_rate_adjust.c PROPERTIES COMPILE_FLAGS "-O3")

set(APP_INCLUDES
    "src"
    "src/vfe_pipeline"
    "src/usb"
    "src/ww_model_runner"
    ${XMOS_RTOS_PLATFORM_INCLUDES}
    ${VFE_LIB_INCLUDES}
)

add_compile_definitions(
    DEBUG_PRINT_ENABLE=1
    XUD_CORE_CLOCK=600
    ENABLE_RTOS_XSCOPE_TRACE=0

    CFG_TUSB_DEBUG_PRINTF=rtos_printf
    CFG_TUSB_DEBUG=0
)

if(USE_WW)
    add_compile_definitions(
        appconfWW_ENABLED=1
    )
    message(STATUS "Using ${USE_WW} model runner")
    include("${CMAKE_SOURCE_DIR}/src/ww_model_runner/${USE_WW}/model_runner.cmake")
endif()

set(APP_LINK_LIBRARIES "${CMAKE_SOURCE_DIR}/src/vfe_pipeline/vfe/libvfe.a")

set(RTOS_TILE_LIST 0 1)
create_multitile_target(RTOS_TILE_LIST)
