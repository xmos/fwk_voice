
add_subdirectory( shared_src )

#add_subdirectory( ic )

add_subdirectory( aec_1_thread )

#add_subdirectory( pipeline_single_threaded )

## Multi thread example builds only on XCORE
if( XCORE )
#add_subdirectory( aec_2_threads )

#add_subdirectory( pipeline_multi_threaded )
endif()

#add_subdirectory( agc )
