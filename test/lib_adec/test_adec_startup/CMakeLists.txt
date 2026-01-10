set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#######################
add_executable(fwk_voice_test_adec_startup)

generate_schedule(fwk_voice_test_adec_startup ${TEST_ADEC_BUILD_CONFIG})

target_link_libraries(fwk_voice_test_adec_startup
    PUBLIC
        fwk_voice::test::shared_test_wav_adec)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_definitions(fwk_voice_test_adec_startup
        PRIVATE
            TRIGGER_DE_ONLY_ON_STARTUP=1
            ENABLE_ADEC_DEBUG_PRINTS=1)

    target_compile_options(fwk_voice_test_adec_startup
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_adec_startup
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
endif()
