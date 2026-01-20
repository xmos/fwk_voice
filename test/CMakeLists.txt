if(USE_CUSTOM_CMAKE)
set( TEST_ONLY_DEPS_ROOT ${CMAKE_SOURCE_DIR}/.. ) #test only dependencies present in the sandbox as part of fwk_voice viewfile
set( TEST_SHARED_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shared )
set( UNITY_PATH ${TEST_ONLY_DEPS_ROOT}/Unity )

include(etc/build_options.cmake)

if ( ${FWK_VOICE_BUILD_TESTS} )
    add_subdirectory( shared )

    add_subdirectory( lib_ns )

    add_subdirectory( lib_aec )

    add_subdirectory( lib_adec )

    add_subdirectory( lib_ic )

    add_subdirectory( lib_agc )

    add_subdirectory( lib_vnr )

    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        add_subdirectory( pipeline )
    endif()

endif()
else()
cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(fwk_voice_tests)
add_subdirectory(lib_agc)
add_subdirectory(lib_ns)
add_subdirectory(lib_ic)
add_subdirectory(lib_vnr)
add_subdirectory(lib_aec)
add_subdirectory(lib_adec)
endif()
