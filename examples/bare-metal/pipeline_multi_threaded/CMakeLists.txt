set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(APP_NAME  avona_example_bare_metal_pipeline_multi_thread)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(${APP_NAME})

target_sources(${APP_NAME}
    PRIVATE
        ${SOURCES_APP})

target_include_directories(${APP_NAME}
    PRIVATE
        src)

target_link_libraries(${APP_NAME}
    PUBLIC
        avona::aec
        avona::adec
        avona::ns
        avona::agc
        avona::ic
        avona::example::aec2thread
        avona::example::delay_buffer
        avona::example::stage_1
        avona::example::fileutils
        avona::example::hpf
        avona::shared::calc_vnr_pred
        core::xs3_math
        )

target_compile_definitions(${APP_NAME}
    PRIVATE
        NUM_AEC_THREADS=2
    )

target_sources(${APP_NAME}
    PRIVATE src/main.xc)

target_compile_options(${APP_NAME}
    PRIVATE "-target=${XCORE_TARGET}")

target_link_options(${APP_NAME}
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")
