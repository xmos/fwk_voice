if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()
#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(avona_test_wav_ns)

file( GLOB_RECURSE SOURCES_C  src/*.c )
target_sources(avona_test_wav_ns
    PRIVATE
        ${SOURCES_C})

target_include_directories(avona_test_wav_ns
    PRIVATE
        src)

target_link_libraries(avona_test_wav_ns
    PRIVATE
        avona::ns
        avona::example::fileutils
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(avona_test_wav_ns
        PRIVATE src/main.xc)

    target_compile_options(avona_test_wav_ns
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_test_wav_ns
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(avona_test_wav_ns
        PRIVATE m)
endif()




