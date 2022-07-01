set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

####### Multi thread alt arch pipeline (Only builds on XCORE)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_executable(fwk_voice_example_bare_metal_pipeline_alt_arch_mt)

    file( GLOB_RECURSE SOURCES_APP  ../pipeline_multi_threaded/src/*.c )
    target_sources(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            ${SOURCES_APP})

    target_include_directories(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            ../pipeline_multi_threaded/src)

    target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PUBLIC
            fwk_voice::aec
            fwk_voice::adec
            fwk_voice::ns
            fwk_voice::agc
            fwk_voice::ic
            fwk_voice::example::aec2thread
            fwk_voice::example::delay_buffer
            fwk_voice::example::stage_1
            fwk_voice::example::fileutils
            fwk_voice::shared::calc_vnr_pred
            core::xs3_math
            )

    target_compile_definitions(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            NUM_AEC_THREADS=2
            ALT_ARCH_MODE=1
        )

    target_sources(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE ../pipeline_multi_threaded/src/main.xc)

    target_compile_options(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_example_bare_metal_pipeline_alt_arch_mt
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
endif()

####### Single thread alt arch pipeline
add_executable(fwk_voice_example_bare_metal_pipeline_alt_arch_st)

file( GLOB_RECURSE SOURCES_APP  ../pipeline_single_threaded/src/*.c )
target_sources(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        ${SOURCES_APP})

target_include_directories(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        ../pipeline_single_threaded/src)

target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PUBLIC
        fwk_voice::aec
        fwk_voice::adec
        fwk_voice::ns
        fwk_voice::agc
        fwk_voice::ic
        fwk_voice::example::aec1thread
        fwk_voice::example::delay_buffer
        fwk_voice::example::stage_1
        fwk_voice::example::fileutils
        fwk_voice::shared::calc_vnr_pred
        core::xs3_math
        )

target_compile_definitions(fwk_voice_example_bare_metal_pipeline_alt_arch_st
    PRIVATE
        NUM_AEC_THREADS=1
        ALT_ARCH_MODE=1
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_sources(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE ../pipeline_single_threaded/src/main.xc)

    target_compile_options(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CONFIG_XSCOPE_PATH}/config.xscope")
else()
    target_link_libraries(fwk_voice_example_bare_metal_pipeline_alt_arch_st
        PRIVATE m)
endif()
