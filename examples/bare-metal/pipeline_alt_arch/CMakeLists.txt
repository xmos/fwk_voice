set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(APP_NAME  avona_example_bare_metal_pipeline_alt_arch)

file( GLOB_RECURSE SOURCES_APP  ../pipeline_multi_threaded/src/*.c )
add_executable(${APP_NAME})

target_sources(${APP_NAME} 
    PRIVATE
        ${SOURCES_APP})

target_include_directories(${APP_NAME} 
    PRIVATE
        ../pipeline_multi_threaded/src)

target_link_libraries(${APP_NAME}
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
        )

target_compile_definitions(${APP_NAME}
    PRIVATE
        NUM_AEC_THREADS=2
        ALT_ARCH_MODE=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(${APP_NAME}
        PRIVATE ../pipeline_multi_threaded/src/main.xc)

    target_compile_options(${APP_NAME}
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(${APP_NAME}
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
