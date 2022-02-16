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

    set ( EXEC test_adec_profile_${CONFIG_STR} )
    add_executable(${EXEC} ${SOURCES_APP} 
                           ${SOURCES_AEC_PROCESS_FRAME}
                           ${AUTOGEN_SOURCES}) 

    target_link_libraries(${EXEC} lib_aec
                                  lib_adec
                                  file_utils
                                  delay_buffer
                                  profile)

    target_include_directories(${EXEC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src
                                               ${SHARED_SRC_PATH}/aec
                                               ${AUTOGEN_DIR})
    if(XCORE)
      target_sources(${EXEC} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src/main.xc)

      target_compile_options(${EXEC} PRIVATE "-DTEST_WAV_XSCOPE=1"
                                             "-DPROFILE_PROCESSING=1"
                                             "-target=${XCORE_TARGET}")

      target_link_options(${EXEC} PRIVATE "-target=${XCORE_TARGET}"
                                          "-report"
                                          "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")

      set_target_properties(${EXEC} PROPERTIES SUFFIX ".xe")
    else()
      target_link_libraries(${EXEC} m)
    endif()
endforeach( BUILD_CONFIG ${TEST_ADEC_PROFILE_BUILD_CONFIG} )
