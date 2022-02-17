
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

## Note this should be a top level config, IE build entire repo for a particular XCORE_TARGET
set(XCORE_TARGET "XCORE-AI-EXPLORER")

add_executable(avona_example_bare_metal_aec_2_thread EXCLUDE_FROM_ALL)
target_sources(avona_example_bare_metal_aec_2_thread
    PRIVATE
        src/aec_app.c
        src/aec_task_distribution.c
        src/main.xc
)
target_include_directories(avona_example_bare_metal_aec_2_thread
    PRIVATE
        src
)
target_compile_options(avona_example_bare_metal_aec_2_thread
    PRIVATE
        "-target=${XCORE_TARGET}"
)
target_compile_definitions(avona_example_bare_metal_aec_2_thread
    PRIVATE
        TEST_WAV_XSCOPE=1
)
target_link_libraries(avona_example_bare_metal_aec_2_thread
    PUBLIC
        avona::example::aec2thread
        avona::example::fileutils_xcore
)
target_link_options(avona_example_bare_metal_aec_2_thread
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
)
