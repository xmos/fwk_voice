
## Defines for tests

## The unit test apps
if(USE_CUSTOM_CMAKE)
    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        add_subdirectory( test_aec_enhancements )
        add_subdirectory( test_aec_spec )
        add_subdirectory( aec_unit_tests )
    endif()
else()
    cmake_minimum_required(VERSION 3.21)
    include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
    project(fwk_voice_tests_aec)
    add_subdirectory(test_aec_xccm)
endif()
