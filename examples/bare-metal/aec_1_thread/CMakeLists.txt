
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

## Note this should be a top level config, IE build entire repo for a particular XCORE_TARGET
set(XCORE_TARGET "XCORE-AI-EXPLORER")

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_executable(avona_example_bare_metal_aec_1_thread EXCLUDE_FROM_ALL)
    target_sources(avona_example_bare_metal_aec_1_thread
        PRIVATE
            src/aec_app.c
            src/main.xc
    )
    target_include_directories(avona_example_bare_metal_aec_1_thread
        PRIVATE
            src
    )
    target_compile_options(avona_example_bare_metal_aec_1_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_compile_definitions(avona_example_bare_metal_aec_1_thread
        PRIVATE
            TEST_WAV_XSCOPE=1
    )
    target_link_libraries(avona_example_bare_metal_aec_1_thread
        PUBLIC
            avona::example::aec1thread
            avona::example::fileutils_xcore
    )
    target_link_options(avona_example_bare_metal_aec_1_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
    )
else()
    add_executable(avona_example_bare_metal_aec_1_thread EXCLUDE_FROM_ALL)
    target_sources(avona_example_bare_metal_aec_1_thread
        PRIVATE
            src/aec_app.c
    )
    target_link_libraries(avona_example_bare_metal_aec_1_thread
        PUBLIC
            avona::example::aec1thread
            avona::example::fileutils_host
    )
    target_include_directories(avona_example_bare_metal_aec_1_thread
        PRIVATE
            src
    )
    target_link_libraries(avona_example_bare_metal_aec_1_thread
        PRIVATE
            m
    )
endif()
