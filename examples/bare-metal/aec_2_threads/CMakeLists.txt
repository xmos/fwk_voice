
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_example_bare_metal_aec_2_thread)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
target_sources(fwk_voice_example_bare_metal_aec_2_thread
    PRIVATE
        ${SOURCES_APP}
)

target_include_directories(fwk_voice_example_bare_metal_aec_2_thread
    PRIVATE
        src
)
target_link_libraries(fwk_voice_example_bare_metal_aec_2_thread
    PUBLIC
        fwk_voice::example::aec2thread
        fwk_voice::example::fileutils
)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_bare_metal_aec_2_thread
        PRIVATE
            src/main.xc
    )
    target_compile_options(fwk_voice_example_bare_metal_aec_2_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(fwk_voice_example_bare_metal_aec_2_thread
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope"
    )
else()
    target_link_libraries(fwk_voice_example_bare_metal_aec_2_thread
        PRIVATE
            m
    )
endif()
