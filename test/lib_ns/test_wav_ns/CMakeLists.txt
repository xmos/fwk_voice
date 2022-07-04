if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()
#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable(fwk_voice_test_wav_ns)

file( GLOB_RECURSE SOURCES_C  src/*.c )
target_sources(fwk_voice_test_wav_ns
    PRIVATE
        ${SOURCES_C})

target_include_directories(fwk_voice_test_wav_ns
    PRIVATE
        src)

target_link_libraries(fwk_voice_test_wav_ns
    PRIVATE
        fwk_voice::ns
        fwk_voice::example::fileutils
        )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_test_wav_ns
        PRIVATE src/main.xc)

    target_compile_options(fwk_voice_test_wav_ns
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_wav_ns
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(fwk_voice_test_wav_ns
        PRIVATE m)
endif()




