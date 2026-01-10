
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

####################
add_executable(fwk_voice_test_delay_estimator)

generate_schedule(fwk_voice_test_delay_estimator ${TEST_DELAY_ESTIMATOR_BUILD_CONFIG})

target_link_libraries(fwk_voice_test_delay_estimator
    PUBLIC
        fwk_voice::test::shared_test_wav_adec)

target_compile_definitions(fwk_voice_test_delay_estimator
    PRIVATE
        ENABLE_ADEC_DEBUG_PRINTS
        BYPASS_ADEC=1)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_test_delay_estimator
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_test_delay_estimator
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
endif()
