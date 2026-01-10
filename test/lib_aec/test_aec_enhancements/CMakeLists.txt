
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#######################
add_executable(fwk_voice_test_aec_enhancements)

generate_schedule(fwk_voice_test_aec_enhancements ${TEST_AEC_ENHANCEMENTS_BUILD_CONFIG})

target_link_libraries(fwk_voice_test_aec_enhancements
    PUBLIC
        fwk_voice::test::shared_test_wav_adec)

target_compile_definitions(fwk_voice_test_aec_enhancements
    PRIVATE
        BYPASS_ADEC=1)


if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_test_aec_enhancements
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_aec_enhancements
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endif()
