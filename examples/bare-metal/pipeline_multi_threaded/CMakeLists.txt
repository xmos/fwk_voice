set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(avona_example_bare_metal_pipeline_multi_thread)

target_sources(avona_example_bare_metal_pipeline_multi_thread 
    PRIVATE
        ${SOURCES_APP})

target_include_directories(avona_example_bare_metal_pipeline_multi_thread 
    PRIVATE
        src)

target_link_libraries(avona_example_bare_metal_pipeline_multi_thread
    PUBLIC
        avona::aec
        avona::adec
        avona::ns
        avona::agc
        avona::example::aec2thread
        avona::example::delay_buffer
        avona::example::stage_1
        avona::example::fileutils
        )

target_compile_definitions(avona_example_bare_metal_pipeline_multi_thread
    PRIVATE
        NUM_AEC_THREADS=2
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_pipeline_multi_thread
        PRIVATE src/main.xc)

    target_compile_options(avona_example_bare_metal_pipeline_multi_thread
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_example_bare_metal_pipeline_multi_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
