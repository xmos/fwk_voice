set( DEPS_ROOT ${CMAKE_SOURCE_DIR}/.. )
set( SHARED_SRC_PATH ${CMAKE_SOURCE_DIR}/examples/bare-metal/shared_src )
set( UNITY_PATH ${CMAKE_BINARY_DIR}/avona_deps/Unity )

add_subdirectory( shared )

add_subdirectory( lib_ns )

add_subdirectory( lib_aec )

add_subdirectory( lib_adec )

add_subdirectory( lib_ic )

add_subdirectory( lib_agc )

add_subdirectory( lib_vad )
