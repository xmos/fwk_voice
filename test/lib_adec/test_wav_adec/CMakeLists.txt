## App name
set( APP_NAME  test_wav_adec )

# Auto-generate schedule and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../../lib_aec/shared_src/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "${APP_NAME} build config:  ${TEST_WAV_ADEC_BUILD_CONFIG}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${TEST_WAV_ADEC_BUILD_CONFIG})

file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
OUTPUT ${GEN_SCHEDULE_SCRIPT_BYPRODUCTS}
COMMAND ${Python3_EXECUTABLE} ${GEN_SCHEDULE_SCRIPT} ${GEN_SCHEDULE_SCRIPT_ARGS}
COMMENT "Generating AEC task distribution and top level config" )


## Depends on libraries
list( APPEND  DEP_LIBS_XCORE  ""  )
list( APPEND  DEP_LIBS_Linux  m   )

list( APPEND  DEP_LIBS        
    lib_aec 
    lib_adec
    lib_adec
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)


## Sources
file( GLOB_RECURSE SOURCES_TEST_WAV_C src/*.c )
file( GLOB SOURCES_TEST_WAV_XC src/*.xc )
file( GLOB SOURCES_DELAY_BUFFER  ${SHARED_SRC_PATH}/delay_buffer/*.c )
if ( XCORE )
    file( GLOB SOURCES_AEC_PROCESS_FRAME  ${SHARED_SRC_PATH}/aec/*.c )
else()
    ## Only 1 thread process_frame() builds for x86
    file( GLOB SOURCES_AEC_PROCESS_FRAME ${SHARED_SRC_PATH}/aec/aec_process_frame_1thread.c )
endif()
file( GLOB_RECURSE FILE_UTILS_SOURCES ${SHARED_SRC_PATH}/file_utils/*.c )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${XSCOPE_FILEIO_PATH}/src/*.c )

unset( SOURCES )
list( APPEND  SOURCES ${SOURCES_TEST_WAV_C} ${SOURCES_AEC_PROCESS_FRAME} ${FILE_UTILS_SOURCES} ${AUTOGEN_SOURCES} ${SOURCES_DELAY_BUFFER})
unset( SOURCES_XCORE )
list( APPEND SOURCES_XCORE ${SOURCES_TEST_WAV_XC} ${XSCOPE_FILEIO_SOURCES} )
list( APPEND SOURCES ${SOURCES_${CMAKE_SYSTEM_NAME}} )

## Includes
unset( INCLUDES )
list( APPEND  INCLUDES src ${SHARED_SRC_PATH}/aec ${SHARED_SRC_PATH}/file_utils ${AUTOGEN_DIR} ${SHARED_SRC_PATH}/delay_buffer)
unset( INCLUDES_XCORE )
list( APPEND INCLUDES_XCORE ${XSCOPE_FILEIO_PATH} ${XSCOPE_FILEIO_PATH}/api )
list( APPEND INCLUDES ${INCLUDES_${CMAKE_SYSTEM_NAME}} )


# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)

list(APPEND   COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1 -DENABLE_ADEC_DEBUG_PRINTS )

unset(COMPILE_FLAGS_Linux)
list( APPEND  COMPILE_FLAGS_Linux   ""  )


##Linker flags
unset(LINKER_FLAGS)
list( APPEND  LINKER_FLAGS  "" )

unset(LINKER_FLAGS_XCORE)
list( APPEND  LINKER_FLAGS_XCORE  "-target=${XCORE_TARGET}"     )
list( APPEND  LINKER_FLAGS_XCORE  "-report"                     )
list( APPEND  LINKER_FLAGS_XCORE  "${XSCOPE_CONFIG}"            )

unset(LINKER_FLAGS_Linux)
list( APPEND  LINKER_FLAGS_Linux  "" )

list( APPEND  LINKER_FLAGS ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )
list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )

#########
## executable output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_executable( ${APP_NAME} ${SOURCES} ${SOURCES_C_APP} )

target_include_directories( ${APP_NAME} PRIVATE ${INCLUDES} )

target_link_libraries( ${APP_NAME} ${DEP_LIBS})

target_compile_options( ${APP_NAME} PRIVATE ${COMPILE_FLAGS} )

#(because otherwise the set_target_properties command fails)
string(REPLACE ";" " " LINKER_FLAGS_STR "${LINKER_FLAGS}")
set_target_properties( ${APP_NAME} PROPERTIES LINK_FLAGS "${LINKER_FLAGS_STR}" )

if ( XCORE )
  set_target_properties( ${APP_NAME} PROPERTIES
      SUFFIX ".xe"
      LINK_DEPENDS  ${XSCOPE_CONFIG}
      )
endif()