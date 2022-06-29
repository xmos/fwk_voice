
set( CONFIG_XSCOPE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/shared_src/etc )

add_subdirectory( shared_src )

add_subdirectory( ic )
add_subdirectory( vad )
add_subdirectory( aec_1_thread )
add_subdirectory( agc )
add_subdirectory( pipeline_single_threaded )
add_subdirectory( pipeline_alt_arch )

add_subdirectory(vnr)
## Multi thread example builds only on XCORE
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    #TODO move vad back to all examples when we can build it
    add_subdirectory( aec_2_threads )

    add_subdirectory( pipeline_multi_threaded )
endif()

