
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
add_executable(fwk_voice_example_bare_metal_vad)

file( GLOB_RECURSE SOURCES_VAD_APP  src/*.c )
target_sources(fwk_voice_example_bare_metal_vad
    PRIVATE
        ${SOURCES_VAD_APP})

target_include_directories(fwk_voice_example_bare_metal_vad
    PRIVATE
        src)
        
target_link_libraries(fwk_voice_example_bare_metal_vad
    PUBLIC
        fwk_voice::vad
        fwk_voice::example::fileutils
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_bare_metal_vad
        PRIVATE
            src/main.xc)

    target_link_libraries(fwk_voice_example_bare_metal_vad
        PUBLIC
            fwk_voice::example::profile_xcore
    )
    target_compile_options(fwk_voice_example_bare_metal_vad
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(fwk_voice_example_bare_metal_vad
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope"
    )
else()
    target_link_libraries(fwk_voice_example_bare_metal_vad
        PRIVATE
            m
    )
endif()
