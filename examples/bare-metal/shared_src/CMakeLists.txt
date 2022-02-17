

# add_subdirectory( file_utils )

# add_subdirectory( delay_buffer )
#
# if(XCORE)
#     add_subdirectory( profile )
# endif()



add_library(avona_example_shared_src_aec_1_thread INTERFACE)
target_sources(avona_example_shared_src_aec_1_thread
    INTERFACE
        aec/aec_process_frame_1thread.c
)
target_include_directories(avona_example_shared_src_aec_1_thread
    INTERFACE
        aec
)
target_link_libraries(avona_example_shared_src_aec_1_thread
    INTERFACE
        avona::aec
)
add_library(avona::example::aec1thread ALIAS avona_example_shared_src_aec_1_thread)


add_library(avona_example_shared_src_aec_2_thread INTERFACE)
target_sources(avona_example_shared_src_aec_2_thread
    INTERFACE
        aec/aec_process_frame_2threads.c
)
target_include_directories(avona_example_shared_src_aec_2_thread
    INTERFACE
        aec
)
target_link_libraries(avona_example_shared_src_aec_2_thread
    INTERFACE
        avona::aec
)
add_library(avona::example::aec2thread ALIAS avona_example_shared_src_aec_2_thread)


add_library(avona_example_shared_src_file_utils_xcore INTERFACE)
target_sources(avona_example_shared_src_file_utils_xcore
    INTERFACE
        file_utils/src/fileio.c
        file_utils/src/wav_utils.c
        xscope_fileio/xscope_fileio/src/xscope_io_device.c
)
target_include_directories(avona_example_shared_src_file_utils_xcore
    INTERFACE
        file_utils/src
        xscope_fileio/xscope_fileio
        xscope_fileio/xscope_fileio/api
)
target_compile_definitions(avona_example_shared_src_file_utils_xcore
    INTERFACE
        TEST_WAV_XSCOPE
)

add_library(avona::example::fileutils_xcore ALIAS avona_example_shared_src_file_utils_xcore)
