add_executable(avona_example_bare_metal_vad)

file( GLOB_RECURSE SOURCES_VAD_APP  src/*.c )
target_sources(avona_example_bare_metal_vad
    PRIVATE
        ${SOURCES_VAD_APP})

target_include_directories(avona_example_bare_metal_vad
    PRIVATE
        src)
        
target_link_libraries(avona_example_bare_metal_vad
    PUBLIC
        avona::vad
        avona::example::fileutils
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_vad
        PRIVATE
            src/main.xc)

    target_link_libraries(avona_example_bare_metal_vad
        PUBLIC
            avona::example::profile_xcore
    )
    target_compile_options(avona_example_bare_metal_vad
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(avona_example_bare_metal_vad
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
    )
else()
    target_link_libraries(avona_example_bare_metal_vad
        PRIVATE
            m
    )
endif()
