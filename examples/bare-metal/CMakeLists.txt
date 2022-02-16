
add_subdirectory( ic )

add_subdirectory( aec_1_thread )
add_subdirectory( vad )
add_subdirectory( agc )
add_subdirectory( pipeline_single_threaded )

## AEC 2 thread example builds only on XCORE
if( XCORE )
add_subdirectory( vad )
#TODO move vad back to all aexamples when we can build it
add_subdirectory( aec_2_threads )
add_subdirectory( pipeline_multi_threaded )
endif()

