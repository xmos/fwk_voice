
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_example_bare_metal_aec_1_thread)

target_sources(avona_example_bare_metal_aec_1_thread
    PRIVATE
        src/aec_app.c
)

target_include_directories(avona_example_bare_metal_aec_1_thread
    PRIVATE
        src
)

target_link_libraries(avona_example_bare_metal_aec_1_thread
    PUBLIC
        avona::example::aec1thread
        core::xs3_math
        avona::example::fileutils
)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_aec_1_thread
        PRIVATE
            src/main.xc
    )
    target_compile_options(avona_example_bare_metal_aec_1_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(avona_example_bare_metal_aec_1_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope"
    )
else()
    target_link_libraries(avona_example_bare_metal_aec_1_thread
        PRIVATE
            m
    )
endif()
