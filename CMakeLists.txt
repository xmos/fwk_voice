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
project(sw_xvf3652)
set(PROJECT_VERSION 0.5.0)

## Enable languages for project
enable_language(CXX C ASM)

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
    "src/vfe_pipeline/burn_cycles.S"
)

set(PIPELINE_LIB_INCLUDES
    "src"
    "src/vfe_pipeline"
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

    "src/platform/driver_instances.c"
    "src/platform/platform_init.c"
    "src/platform/platform_start.c"
    "src/platform/app_pll_ctrl.c"
    "src/platform/aic3204.c"

    "src/app_control/app_control.c"

    "src/usb/usb_audio.c"
    "src/usb/usb_descriptors.c"
    "src/usb/adaptive_rate_adjust.c"

    "src/spi/spi_interface.c"

    "src/ww_model_runner/ww_model_runner.c"

    "src/gpio_test/gpio_test.c"

    # "$ENV{WW_PATH}/models/common/WR_250k.en-US.alexa.cpp"
)

set_source_files_properties(src/usb/adaptive_rate_adjust.c PROPERTIES COMPILE_FLAGS "-O3")

set(APP_INCLUDES
    "src"
    "src/usb"
    "src/ww_model_runner"

    "$ENV{WW_PATH}/xs3a"
)

add_compile_definitions(
    DEBUG_PRINT_ENABLE=1
    PLATFORM_USES_TILE_0=1
    PLATFORM_USES_TILE_1=1
    XUD_CORE_CLOCK=700
    ENABLE_RTOS_XSCOPE_TRACE=0

    CFG_TUSB_DEBUG_PRINTF=rtos_printf
    CFG_TUSB_DEBUG=0

    # PRL_MODEL_ALIGN=\_\_attribute\_\_\(\(aligned\(8\)\)\)\_\_attribute\_\_\(\(section\(\".ExtMem_data\"\)\)\)
)

add_executable(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} "${CMAKE_SOURCE_DIR}/src/vfe_pipeline/vfe/libvfe.a")
target_link_libraries(${TARGET_NAME} lib_vfe_pipeline)
target_link_libraries(${TARGET_NAME} "$ENV{WW_PATH}/xs3a/U/libpryon_lite-U.a")

target_sources(${TARGET_NAME} PRIVATE ${APP_SOURCES} ${XMOS_RTOS_PLATFORM_SOURCES})
target_include_directories(${TARGET_NAME} PRIVATE ${APP_INCLUDES} ${XMOS_RTOS_PLATFORM_INCLUDES})

target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
