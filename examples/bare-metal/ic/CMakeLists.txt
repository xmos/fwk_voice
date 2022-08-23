
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

############################
add_library(fwk_voice_example_shared_ic_test_wav INTERFACE)
file( GLOB_RECURSE SOURCES_IC_APP  src/*.c )
target_sources(fwk_voice_example_shared_ic_test_wav
    INTERFACE
        ${SOURCES_IC_APP})
target_include_directories(fwk_voice_example_shared_ic_test_wav
    INTERFACE
        src)
target_link_libraries(fwk_voice_example_shared_ic_test_wav
    INTERFACE
        fwk_voice::ic
        fwk_voice::example::fileutils
        )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_shared_ic_test_wav
        INTERFACE
            src/main.xc
    )
    target_link_libraries(fwk_voice_example_shared_ic_test_wav
        INTERFACE
            fwk_voice::example::profile_xcore
            )
endif()
add_library(fwk_voice::example::test_wav_ic ALIAS fwk_voice_example_shared_ic_test_wav)

############################
add_executable(fwk_voice_example_bare_metal_ic)

target_link_libraries(fwk_voice_example_bare_metal_ic
    PUBLIC
        fwk_voice::example::test_wav_ic
)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(fwk_voice_example_bare_metal_ic
        PRIVATE
            "-target=${XCORE_TARGET}"
    )
    target_link_options(fwk_voice_example_bare_metal_ic
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope"
    )
else()
    target_link_libraries(fwk_voice_example_bare_metal_ic
        PRIVATE
            m
    )
endif()
