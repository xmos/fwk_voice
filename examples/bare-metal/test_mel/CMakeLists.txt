
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_mel)

target_sources(avona_test_mel PRIVATE src/test_mel.c)

target_include_directories(avona_test_mel PRIVATE src)

target_link_libraries(avona_test_mel 
    PUBLIC
        avona::vnr::features
        avona::example::fileutils)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_mel PRIVATE src/main.xc)

    target_compile_options(avona_test_mel
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_test_mel
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
