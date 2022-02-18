set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_example_bare_metal_agc)

target_sources(avona_example_bare_metal_agc
    PRIVATE
        src/agc_app.c)

target_include_directories(avona_example_bare_metal_agc
    PRIVATE
        src)

target_link_libraries(avona_example_bare_metal_agc
    PUBLIC
        avona::agc) 

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_example_bare_metal_agc
        PRIVATE src/main.xc)

    target_link_libraries(avona_example_bare_metal_agc
        PUBLIC avona::example::fileutils_xcore)

    target_compile_options(avona_example_bare_metal_agc
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_example_bare_metal_agc
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(avona_example_bare_metal_agc
        PUBLIC
            avona::example::fileutils_host
    )
    target_link_libraries(avona_example_bare_metal_agc
        PRIVATE m)
endif()
