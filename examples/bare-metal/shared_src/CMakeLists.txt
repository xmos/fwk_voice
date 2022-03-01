######
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


######
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

######
add_library(avona_example_shared_src_delay_buffer  INTERFACE)
target_sources(avona_example_shared_src_delay_buffer
    INTERFACE
        delay_buffer/delay_buffer.c
)
target_include_directories(avona_example_shared_src_delay_buffer
    INTERFACE
        delay_buffer
)
add_library(avona::example::delay_buffer ALIAS avona_example_shared_src_delay_buffer)

######
add_library(avona_example_shared_src_stage_1  INTERFACE)
target_sources(avona_example_shared_src_stage_1
    INTERFACE
        pipeline_stage_1/stage_1.c
)
target_include_directories(avona_example_shared_src_stage_1
    INTERFACE
        pipeline_stage_1
)
add_library(avona::example::stage_1 ALIAS avona_example_shared_src_stage_1)

######
add_library(avona_example_shared_src_file_utils INTERFACE)
target_sources(avona_example_shared_src_file_utils
    INTERFACE
        file_utils/fileio.c
        file_utils/wav_utils.c
)
target_include_directories(avona_example_shared_src_file_utils
    INTERFACE
        file_utils
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_shared_src_file_utils
        INTERFACE
            xscope_fileio/xscope_fileio/src/xscope_io_device.c)

    target_include_directories(avona_example_shared_src_file_utils
        INTERFACE
            xscope_fileio/xscope_fileio
            xscope_fileio/xscope_fileio/api)

    target_compile_definitions(avona_example_shared_src_file_utils
        INTERFACE
            TEST_WAV_XSCOPE=1)
endif()

add_library(avona::example::fileutils ALIAS avona_example_shared_src_file_utils)

######
add_library(avona_example_shared_src_profile_xcore INTERFACE)
target_sources(avona_example_shared_src_profile_xcore
    INTERFACE
        profile/profile.c
)
target_include_directories(avona_example_shared_src_profile_xcore
    INTERFACE
        profile
)

add_library(avona::example::profile_xcore ALIAS avona_example_shared_src_profile_xcore)