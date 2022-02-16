set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${SHARED_SRC_PATH}/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "${APP_NAME} build config:  ${TEST_ADEC_BUILD_CONFIG}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${TEST_ADEC_BUILD_CONFIG})

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
COMMENT "Generating AEC task distribution and top level config" )


file( GLOB_RECURSE SOURCES_APP  ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src/*.c )
if ( XCORE )
    file( GLOB SOURCES_AEC_PROCESS_FRAME  ${SHARED_SRC_PATH}/aec/*.c )
else()
    ## Only 1 thread process_frame() builds for x86
    file( GLOB SOURCES_AEC_PROCESS_FRAME ${SHARED_SRC_PATH}/aec/aec_process_frame_1thread.c )
endif()

add_executable(test_adec ${SOURCES_APP} 
                         ${SOURCES_AEC_PROCESS_FRAME}
                         ${AUTOGEN_SOURCES}) 

target_link_libraries(test_adec lib_aec
                                lib_adec
                                file_utils
                                delay_buffer)

target_include_directories(test_adec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src
                                             ${SHARED_SRC_PATH}/aec
                                             ${AUTOGEN_DIR})
if(XCORE)
  target_sources(test_adec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../test_wav_adec/src/main.xc)

  target_compile_options(test_adec PRIVATE "-DTEST_WAV_XSCOPE=1"
                                           "-DENABLE_ADEC_DEBUG_PRINTS"
                                           "-target=${XCORE_TARGET}")

  target_link_options(test_adec PRIVATE "-target=${XCORE_TARGET}"
                                        "-report"
                                        "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")

  set_target_properties(test_adec PROPERTIES SUFFIX ".xe")
else()
  target_link_libraries(test_adec m)
endif()
