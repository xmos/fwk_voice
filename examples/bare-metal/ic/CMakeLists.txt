
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
file( GLOB_RECURSE SOURCES_IC_APP  src/*.c )

add_executable(avona_example_bare_metal_ic EXCLUDE_FROM_ALL)

target_sources(avona_example_bare_metal_ic
    PRIVATE
        ${SOURCES_IC_APP}
)
target_include_directories(avona_example_bare_metal_ic
    PRIVATE
        src
)
target_link_libraries(avona_example_bare_metal_ic
    PUBLIC
        avona::ic
        avona::example::profile_xcore
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_ic
        PRIVATE
            src/main.xc
    )
    target_link_libraries(avona_example_bare_metal_ic
        PUBLIC
            avona::example::fileutils_xcore
    )
    target_compile_options(avona_example_bare_metal_ic
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(avona_example_bare_metal_ic
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope"
    )
else()
    target_link_libraries(avona_example_bare_metal_ic
        PUBLIC
            avona::example::fileutils_host
    )
    target_link_libraries(avona_example_bare_metal_ic
        PRIVATE
            m
    )
endif()
