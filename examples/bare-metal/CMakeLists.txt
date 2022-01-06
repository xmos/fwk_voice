
set(DEPS_ROOT ${CMAKE_SOURCE_DIR}/..)

add_subdirectory( sup )

add_subdirectory( aec_1_thread )
## AEC 2 thread example builds only on XCORE
if( XCORE )
add_subdirectory( aec_2_threads )
endif()
