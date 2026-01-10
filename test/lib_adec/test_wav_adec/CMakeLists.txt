set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

######################
file(GLOB SOURCES_TEST_WAV src/*.c)
add_library(fwk_voice_test_shared_test_wav_adec INTERFACE)
target_sources(fwk_voice_test_shared_test_wav_adec
    INTERFACE
        ${SOURCES_TEST_WAV})
target_include_directories(fwk_voice_test_shared_test_wav_adec
    INTERFACE
        src)
target_link_libraries(fwk_voice_test_shared_test_wav_adec
    INTERFACE
        fwk_voice::adec
        fwk_voice::example::delay_buffer
        fwk_voice::example::fileutils)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_test_shared_test_wav_adec
        INTERFACE
            src/main.xc)
endif()

add_library(fwk_voice::test::shared_test_wav_adec ALIAS fwk_voice_test_shared_test_wav_adec)

####################
add_executable(fwk_voice_test_wav_adec)

generate_schedule(fwk_voice_test_wav_adec ${TEST_WAV_ADEC_BUILD_CONFIG})

target_link_libraries(fwk_voice_test_wav_adec
    PUBLIC
        fwk_voice::test::shared_test_wav_adec)


if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_test_wav_adec
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_wav_adec
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endif()
