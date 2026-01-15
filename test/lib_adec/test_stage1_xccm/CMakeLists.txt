cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(test_stage1_xccm)

set(APP_HW_TARGET XK-EVK-XU316)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)
set(XMOS_DEP_DIR_lib_voice ${XMOS_SANDBOX_DIR}/fwk_voice)
set(APP_DEPENDENT_MODULES "lib_voice")

XMOS_REGISTER_APP()
