
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

############################
add_library(avona_example_shared_ic_test_wav INTERFACE)
file( GLOB_RECURSE SOURCES_IC_APP  src/*.c )
target_sources(avona_example_shared_ic_test_wav
    INTERFACE
        ${SOURCES_IC_APP})
target_include_directories(avona_example_shared_ic_test_wav
    INTERFACE
        src)
target_link_libraries(avona_example_shared_ic_test_wav
    INTERFACE
        avona::ic
        avona::example::profile_xcore
        avona::example::fileutils
        )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_shared_ic_test_wav
        INTERFACE
            src/main.xc
    )
endif()
add_library(avona::example::test_wav_ic ALIAS avona_example_shared_ic_test_wav)

############################
add_executable(avona_example_bare_metal_ic)

target_link_libraries(avona_example_bare_metal_ic
    PUBLIC
        avona::example::test_wav_ic
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
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
        PRIVATE
            m
    )
endif()
