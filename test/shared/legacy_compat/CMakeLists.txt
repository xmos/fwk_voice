
if((${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS2A))
    ## Create library target
    add_library(fwk_voice_test_deps_legacy_compat INTERFACE)
    target_include_directories(fwk_voice_test_deps_legacy_compat
        INTERFACE
            api
    )
    ## Create an alias
    add_library(fwk_voice_test_deps::legacy_compat ALIAS fwk_voice_test_deps_legacy_compat)
endif()
