
## Defines for tests

## The unit test apps
if(USE_CUSTOM_CMAKE)
    add_subdirectory( test_calc_vnr_pred )
    add_subdirectory( characterise_c_py )
    add_subdirectory( test_ic_profile )
    add_subdirectory( test_ic_spec )
    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        add_subdirectory( ic_unit_tests )
        add_subdirectory( test_bad_state )
    endif()
else()
    cmake_minimum_required(VERSION 3.21)
    include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
    project(fwk_voice_tests_ic)
    add_subdirectory(test_ic_xccm)
endif()
