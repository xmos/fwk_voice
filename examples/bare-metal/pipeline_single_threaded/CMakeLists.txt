set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(avona_example_bare_metal_pipeline_single_thread)

target_sources(avona_example_bare_metal_pipeline_single_thread 
    PRIVATE
        ${SOURCES_APP})

target_include_directories(avona_example_bare_metal_pipeline_single_thread 
    PRIVATE
        src)

target_link_libraries(avona_example_bare_metal_pipeline_single_thread
    PUBLIC
        avona::aec
        avona::adec
        avona::ns
        avona::agc
        avona::ic
        avona::vad
        avona::example::aec1thread
        avona::example::delay_buffer
        avona::example::stage_1
        avona::example::fileutils
        avona::example::hpf
        sdk::lib_xs3_math
        )

target_compile_definitions(avona_example_bare_metal_pipeline_single_thread
    PRIVATE
        NUM_AEC_THREADS=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_pipeline_single_thread
        PRIVATE src/main.xc)

    target_compile_options(avona_example_bare_metal_pipeline_single_thread
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_example_bare_metal_pipeline_single_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # Static linking doesn't work on Mac(ld: library not found for -lcrt0.o)
    target_link_libraries(avona_example_bare_metal_pipeline_single_thread
        PRIVATE m)
else()
    # Note we link this app statically so that we can run it on a different machine
    # that may not have all of the dependancies in the right places
    target_link_libraries(avona_example_bare_metal_pipeline_single_thread
        PRIVATE m "-static")
endif()

