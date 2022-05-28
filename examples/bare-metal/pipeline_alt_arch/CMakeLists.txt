set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

####### Multi thread alt arch pipeline (Only builds on XCORE)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_executable(avona_example_bare_metal_pipeline_alt_arch_mt)

    file( GLOB_RECURSE SOURCES_APP  ../pipeline_multi_threaded/src/*.c )
    target_sources(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            ${SOURCES_APP})

    target_include_directories(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            ../pipeline_multi_threaded/src)

    target_link_libraries(avona_example_bare_metal_pipeline_alt_arch_mt
        PUBLIC
            avona::aec
            avona::adec
            avona::ns
            avona::agc
            avona::ic
            avona::vad
            avona::example::aec2thread
            avona::example::delay_buffer
            avona::example::stage_1
            avona::example::fileutils
            avona::example::hpf
            core::xs3_math
            )

    target_compile_definitions(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            NUM_AEC_THREADS=2
            ALT_ARCH_MODE=1
        )

    target_sources(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE ../pipeline_multi_threaded/src/main.xc)

    target_compile_options(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
endif()

####### Single thread alt arch pipeline
add_executable(avona_example_bare_metal_pipeline_alt_arch_st)

file( GLOB_RECURSE SOURCES_APP  ../pipeline_single_threaded/src/*.c )
target_sources(avona_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        ${SOURCES_APP})

target_include_directories(avona_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        ../pipeline_single_threaded/src)

target_link_libraries(avona_example_bare_metal_pipeline_alt_arch_st
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
        core::xs3_math
        )

target_compile_definitions(avona_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        NUM_AEC_THREADS=1
        ALT_ARCH_MODE=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_pipeline_alt_arch_st
        PRIVATE ../pipeline_single_threaded/src/main.xc)

    target_compile_options(avona_example_bare_metal_pipeline_alt_arch_st
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_example_bare_metal_pipeline_alt_arch_st
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
    target_link_libraries(avona_example_bare_metal_pipeline_alt_arch_st
        PRIVATE m)
endif()
