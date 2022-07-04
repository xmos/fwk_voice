set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${SHARED_SRC_PATH}/python/generate_task_distribution_scheme.py )

## Build each config
foreach( BUILD_CONFIG ${TEST_ADEC_PROFILE_BUILD_CONFIG} )
    # Convert to config str of the form <threads>_<ychannels>_<xchannels>_<mainphases>_<shadowphases>
    string( REPLACE " " "_" CONFIG_STR ${BUILD_CONFIG} )
    message( STATUS "test_adec_profile ${CONFIG_STR}" )
    
    ## Generate schedule for given config
    set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen_${CONFIG_STR} )
    set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
    set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)
    set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )
    unset( GEN_SCHEDULE_SCRIPT_ARGS)
    list( APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
    list( APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${BUILD_CONFIG})
    file( MAKE_DIRECTORY ${AUTOGEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
        COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
        COMMENT "Generating AEC task distribution and top level config" )

    file( GLOB_RECURSE SOURCES_APP  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src/*.c )
    file( GLOB SOURCES_AEC_PROCESS_FRAME  ${SHARED_SRC_PATH}/aec/*.c )

#######
    add_executable(fwk_voice_test_adec_profile_${CONFIG_STR})

    target_sources(fwk_voice_test_adec_profile_${CONFIG_STR}
        PRIVATE
            ${AUTOGEN_SOURCES})

    target_include_directories(fwk_voice_test_adec_profile_${CONFIG_STR}
        PRIVATE
            ${AUTOGEN_DIR})

    target_link_libraries(fwk_voice_test_adec_profile_${CONFIG_STR}
        PUBLIC
            fwk_voice::test::shared_test_wav_adec
            fwk_voice::example::profile_xcore
            )

    target_compile_definitions(fwk_voice_test_adec_profile_${CONFIG_STR}
        PRIVATE
            PROFILE_PROCESSING=1)

    if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
        target_compile_options(fwk_voice_test_adec_profile_${CONFIG_STR}
            PRIVATE "-target=${XCORE_TARGET}")

        target_link_options(fwk_voice_test_adec_profile_${CONFIG_STR}
            PRIVATE
                "-target=${XCORE_TARGET}"
                "-report"
                "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
    else()
        target_link_libraries(fwk_voice_test_adec_profile_${CONFIG_STR}
            PRIVATE m)
    endif()
endforeach()
