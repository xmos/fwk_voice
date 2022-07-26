set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(APP_NAME  fwk_voice_example_bare_metal_pipeline_multi_thread)

file( GLOB_RECURSE SOURCES_APP  src/*.c )
add_executable(${APP_NAME})

target_sources(${APP_NAME}
    PRIVATE
        ${SOURCES_APP})

target_include_directories(${APP_NAME}
    PRIVATE
        src)

target_link_libraries(${APP_NAME}
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

target_compile_definitions(${APP_NAME}
    PRIVATE
        NUM_AEC_THREADS=2
        #DISABLE_STAGE_2=1
        #DISABLE_STAGE_3=1
        #DISABLE_INITIAL_DELAY_EST=1
        #DISABLE_AGC_ADAPT_GAIN=1
    )

target_sources(${APP_NAME}
    PRIVATE src/main.xc)

target_compile_options(${APP_NAME}
    PRIVATE "-target=${XCORE_TARGET}")

target_link_options(${APP_NAME}
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")
