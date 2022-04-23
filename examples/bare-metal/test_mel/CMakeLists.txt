
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_mel)

target_sources(avona_test_mel PRIVATE src/test_mel.c)

target_include_directories(avona_test_mel PRIVATE src)

set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup
    ${CONFIG_XSCOPE_PATH}/config.xscope
    -target=${XCORE_TARGET}
)
target_link_libraries(avona_test_mel 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        avona::example::fileutils)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_mel PRIVATE src/main.xc)

    target_compile_options(avona_test_mel
        PRIVATE ${APP_COMPILER_FLAGS})

    target_link_options(avona_test_mel
        PRIVATE
            -lquadspi
            -w            
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
