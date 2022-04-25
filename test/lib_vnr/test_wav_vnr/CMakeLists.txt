
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_wav_vnr)

target_sources(avona_test_wav_vnr PRIVATE src/test_wav_vnr.c)

target_include_directories(avona_test_wav_vnr PRIVATE src)

set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup
    ${CMAKE_CURRENT_SOURCE_DIR}/config.xscope
    -target=${XCORE_TARGET}
)
target_link_libraries(avona_test_wav_vnr 
    PUBLIC
        avona::vnr::features
        avona::vnr::inference
        avona::example::fileutils)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_wav_vnr PRIVATE src/main.xc)

    target_compile_options(avona_test_wav_vnr
        PRIVATE ${APP_COMPILER_FLAGS})

    target_link_options(avona_test_wav_vnr
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
