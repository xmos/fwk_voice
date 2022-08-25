set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

####### Multi thread alt arch pipeline (Only builds on XCORE)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_executable(fwk_voice_example_bare_metal_pipeline_alt_arch_mt)

    target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PUBLIC
            fwk_voice_example_pipeline_multi_thread_interface
            )

    target_compile_definitions(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            ALT_ARCH_MODE=1
        )

endif()

####### Single thread alt arch pipeline
add_executable(fwk_voice_example_bare_metal_pipeline_alt_arch_st)

target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PUBLIC
        fwk_voice_example_pipeline_single_thread_interface
        )

target_compile_definitions(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        ALT_ARCH_MODE=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_link_options(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
    target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE m)
endif()
