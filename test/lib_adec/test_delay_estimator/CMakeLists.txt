
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${SHARED_SRC_PATH}/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "avona_test_delay_estimator build config:  ${TEST_DELAY_ESTIMATOR_BUILD_CONFIG}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${TEST_DELAY_ESTIMATOR_BUILD_CONFIG})

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
COMMENT "Generating avona_test_delay_estimator task distribution and top level config" )


####################
add_executable(avona_test_delay_estimator)

target_sources(avona_test_delay_estimator
    PRIVATE
        ${AUTOGEN_SOURCES})

target_include_directories(avona_test_delay_estimator
    PRIVATE
        ${AUTOGEN_DIR})

target_link_libraries(avona_test_delay_estimator
    PUBLIC
        avona::test::shared_test_wav_adec)

target_compile_definitions(avona_test_delay_estimator
    PRIVATE
        ENABLE_ADEC_DEBUG_PRINTS
        BYPASS_ADEC=1)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(avona_test_delay_estimator
        PRIVATE "-target=${XCORE_TARGET}")

    target_link_options(avona_test_delay_estimator
        PRIVATE
            "-target=${XCORE_TARGET}"
            "-report"
            "${CMAKE_CURRENT_SOURCE_DIR}/config.xscope")
else()
    target_link_libraries(
        PRIVATE m)
endif()
