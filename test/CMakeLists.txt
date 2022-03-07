set( TEST_ONLY_DEPS_ROOT ${CMAKE_SOURCE_DIR}/.. ) #test only dependencies present in the sandbox as part of sw_avona viewfile
set( SHARED_SRC_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src )
set( UNITY_PATH ${TEST_ONLY_DEPS_ROOT}/Unity )

add_subdirectory( shared )

add_subdirectory( lib_ns )

add_subdirectory( lib_aec )

add_subdirectory( lib_adec )

add_subdirectory( lib_ic )

add_subdirectory( lib_agc )

add_subdirectory( lib_vad )
