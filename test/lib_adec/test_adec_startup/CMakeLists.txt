set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${SHARED_SRC_PATH}/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "fwk_voice_test_adec_startup_startup build config:  ${TEST_ADEC_BUILD_CONFIG}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${TEST_ADEC_BUILD_CONFIG})

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
COMMENT "Generating fwk_voice_test_adec_startup task distribution and top level config" )

#######################
add_executable(fwk_voice_test_adec_startup)

target_sources(fwk_voice_test_adec_startup
    PRIVATE
        ${AUTOGEN_SOURCES})

target_include_directories(fwk_voice_test_adec_startup
    PRIVATE
        ${AUTOGEN_DIR})

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
else()
    target_link_libraries(
        PRIVATE m)
endif()
