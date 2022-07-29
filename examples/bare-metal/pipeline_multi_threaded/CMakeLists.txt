
### Add interface library
add_library(fwk_voice_example_pipeline_multi_thread_interface INTERFACE)
file( GLOB_RECURSE SOURCES_APP  src/*.c )

target_sources(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE
        ${SOURCES_APP})

target_include_directories(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE
        src)

target_link_libraries(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE
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

target_compile_definitions(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE
        NUM_AEC_THREADS=2
    )

target_sources(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE src/main.xc)

target_compile_options(fwk_voice_example_pipeline_multi_thread_interface
    INTERFACE "-target=${XCORE_TARGET}")

### Add executable
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
add_executable(fwk_voice_example_bare_metal_pipeline_multi_thread)

target_link_libraries(fwk_voice_example_bare_metal_pipeline_multi_thread
    PUBLIC
        fwk_voice_example_pipeline_multi_thread_interface
        )

target_link_options(fwk_voice_example_bare_metal_pipeline_multi_thread
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")

### Build AEC + IC pipeline for testing against python AEC+IC pipeline
add_executable(fwk_voice_example_pipeline_aec_ic)

target_link_libraries(fwk_voice_example_pipeline_aec_ic
    PUBLIC
        fwk_voice_example_pipeline_multi_thread_interface
        )

target_compile_definitions(fwk_voice_example_pipeline_aec_ic
    PRIVATE
        DISABLE_STAGE_3=1
        DISABLE_INITIAL_DELAY_EST=1
        #DISABLE_AGC_ADAPT_GAIN=1
    )

target_link_options(fwk_voice_example_pipeline_aec_ic
    PRIVATE
        "-target=${XCORE_TARGET}"
        "-report"
        "${CONFIG_XSCOPE_PATH}/config.xscope")

