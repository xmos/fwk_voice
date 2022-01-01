## App name
set( APP_NAME  test_wav_aec )

# Auto-generate task distribution scheme and top level config files

if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

set( GEN_SCHEDULE_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../shared_src/python/generate_task_distribution_scheme.py )
set( AUTOGEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src.autogen )
set( AUTOGEN_SOURCES ${AUTOGEN_DIR}/aec_task_distribution.c )
set( AUTOGEN_INCLUDES ${AUTOGEN_DIR}/aec_task_distribution.h ${AUTOGEN_DIR}/aec_config.h)

message(STATUS "${APP_NAME} build config:  ${TEST_WAV_AEC_BUILD_CONFIG}" )
set( GEN_SCHEDULE_SCRIPT_BYPRODUCTS ${AUTOGEN_SOURCES} ${AUTOGEN_INCLUDES} )

unset(GEN_SCHEDULE_SCRIPT_ARGS) 
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --out-dir ${AUTOGEN_DIR})
list(APPEND GEN_SCHEDULE_SCRIPT_ARGS --config ${TEST_WAV_AEC_BUILD_CONFIG})

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
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}}
)

list( APPEND TEST_AUTOGEN_SOURCES ${AUTOGEN_SOURCES} )
list( APPEND TEST_AUTOGEN_INCLUDES ${AUTOGEN_INCLUDES} )

## Sources
file( GLOB SOURCES_C  src/*.c )
file( GLOB SOURCES_XC src/*.xc )
file( GLOB SOURCES_C_APP  src/testapp_c/*.c )
file( GLOB SOURCES_XC_APP  src/testapp_xc/*.xc )
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${DEPS_ROOT}/xscope_fileio/xscope_fileio/src/*.c )

unset( SOURCES )
list( APPEND  SOURCES ${SOURCES_C}  ${AUTOGEN_SOURCES} )
unset( SOURCES_XCORE )
list( APPEND SOURCES_XCORE ${SOURCES_XC} ${XSCOPE_FILEIO_SOURCES} )
list( APPEND SOURCES ${SOURCES_${CMAKE_SYSTEM_NAME}} )

## Includes
unset( INCLUDES )
list( APPEND  INCLUDES src ${CMAKE_CURRENT_SOURCE_DIR}/../shared_src ${AUTOGEN_DIR} )
unset( INCLUDES_XCORE )
list( APPEND INCLUDES_XCORE ${DEPS_ROOT}/xscope_fileio/xscope_fileio ${DEPS_ROOT}/xscope_fileio/xscope_fileio/api )
list( APPEND INCLUDES ${INCLUDES_${CMAKE_SYSTEM_NAME}} )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
unset(COMPILE_FLAGS_XCORE)

list(APPEND   COMPILE_FLAGS_XCORE  -DTEST_WAV_XSCOPE=1 )

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

set( C_APP_EXECUTABLE ${APP_NAME}_c_app )

add_executable( ${C_APP_EXECUTABLE} ${SOURCES} ${SOURCES_C_APP} )

target_include_directories( ${C_APP_EXECUTABLE} PRIVATE ${INCLUDES} )

target_link_libraries( ${C_APP_EXECUTABLE} ${DEP_LIBS})

target_compile_options( ${C_APP_EXECUTABLE} PRIVATE ${COMPILE_FLAGS} )

#(because otherwise the set_target_properties command fails)
string(REPLACE ";" " " LINKER_FLAGS_STR "${LINKER_FLAGS}")
set_target_properties( ${C_APP_EXECUTABLE} PROPERTIES LINK_FLAGS "${LINKER_FLAGS_STR}" )

if ( XCORE )
  set_target_properties( ${C_APP_EXECUTABLE} PROPERTIES
      SUFFIX ".xe"
      LINK_DEPENDS  ${XSCOPE_CONFIG}
      )
endif()
