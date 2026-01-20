
if(USE_CUSTOM_CMAKE)
    add_subdirectory(vnr_unit_tests)

    add_subdirectory(test_vnr_profile)
else()
    cmake_minimum_required(VERSION 3.21)
    include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
    project(fwk_voice_tests_vnr)
    add_subdirectory(test_vnr_xccm)
endif()
