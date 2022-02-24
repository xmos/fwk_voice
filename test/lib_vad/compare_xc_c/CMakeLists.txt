## This cmake file builds the "old vad" testwav example. Note inclusion of lots
## of source files from the old repos which do not support cmake libs
## App name
set( APP_NAME  run_old_vad )


#These deps are present in the test view only
set( DEPS_ROOT ../../../..)

## Sources
file( GLOB_RECURSE SOURCES_C  src/*.c )
file( GLOB_RECURSE SOURCES_XC  src/*.xc )
file( GLOB_RECURSE  SOURCES_RAND  ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand/*.c )


## LIB_DSP
set(LIB_DSP_PATH ${XCORE_SDK_PATH}/modules/lib_dsp/lib_dsp )
file( GLOB_RECURSE LIB_DSP_SOURCES_C  ${LIB_DSP_PATH}/src/*.c )
file( GLOB_RECURSE LIB_DSP_SOURCES_XC  ${LIB_DSP_PATH}/src/*.xc )
file( GLOB_RECURSE LIB_DSP_SOURCES_ASM  ${LIB_DSP_PATH}/src/*.S )
set( LIB_DSP_SOURCES  ${LIB_DSP_SOURCES_C} ${LIB_DSP_SOURCES_XC} ${LIB_DSP_SOURCES_ASM})
set( LIB_DSP_INCLUDES  ${LIB_DSP_PATH}/api/)

## LIB_AI
file( GLOB_RECURSE LIB_AI_SOURCES_C  ${DEPS_ROOT}/lib_ai/lib_ai/src/*.c )
file( GLOB_RECURSE LIB_AI_SOURCES_XC  ${DEPS_ROOT}/lib_ai/lib_ai/src/*.xc )
file( GLOB_RECURSE LIB_AI_SOURCES_ASM  ${DEPS_ROOT}/lib_ai/lib_ai/src/*.S )
set( LIB_AI_SOURCES  ${LIB_AI_SOURCES_C} ${LIB_AI_SOURCES_XC} ${LIB_AI_SOURCES_ASM})
set( LIB_AI_INCLUDES ${DEPS_ROOT}/lib_ai/lib_ai/api/ ${DEPS_ROOT}/lib_ai/lib_ai/src/)

## LIB_VAD
file( GLOB_RECURSE LIB_VAD_SOURCES_C  ${DEPS_ROOT}/lib_vad/lib_vad/src/*.c )
file( GLOB_RECURSE LIB_VAD_SOURCES_XC  ${DEPS_ROOT}/lib_vad/lib_vad/src/*.xc )
file( GLOB_RECURSE LIB_VAD_SOURCES_ASM  ${DEPS_ROOT}/lib_vad/lib_vad/src/*.S )
set( LIB_VAD_SOURCES  ${LIB_VAD_SOURCES_C} ${LIB_VAD_SOURCES_XC} ${LIB_VAD_SOURCES_ASM})
list(REMOVE_ITEM LIB_VAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${DEPS_ROOT}/lib_vad/lib_vad/src/vad_test_task.xc)

set( LIB_VAD_INCLUDES ${DEPS_ROOT}/lib_vad/lib_vad/api/ ${DEPS_ROOT}/lib_vad/lib_vad/src/)

## Helpers
file( GLOB_RECURSE XSCOPE_FILEIO_SOURCES  ${XSCOPE_FILEIO_PATH}/src/*.c )
list( APPEND XSCOPE_FILEIO_INCLUDES  ${XSCOPE_FILEIO_PATH}/api/ ${XSCOPE_FILEIO_PATH}/ ../../../examples/bare-metal/shared_src/file_utils/)
file( GLOB SOURCES_FILE_UTILS ${SHARED_SRC_PATH}/file_utils/*.c ) 



list( APPEND  SOURCES_XCORE     ${SOURCES_C} ${SOURCES_XC} ${SOURCES_RAND}
                                ${SOURCES_FILE_UTILS} ${SOURCES_TEST} ${XSCOPE_FILEIO_SOURCES}
                                ${LIB_DSP_SOURCES} ${LIB_AI_SOURCES} ${LIB_VAD_SOURCES} )
list( APPEND  SOURCES   ${SOURCES_${CMAKE_SYSTEM_NAME}} )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG ./config.xscope ABSOLUTE)
message( STATUS "XSCOPE_CONFIG ${XSCOPE_CONFIG}" )


## Compile flags
unset(COMPILE_FLAGS)
list( APPEND  COMPILE_FLAGS   -Wno-unused-variable )

unset(COMPILE_FLAGS_XCORE)
list( APPEND  COMPILE_FLAGS_XCORE -DTEST_WAV_XSCOPE=1 -target=${XCORE_TARGET})

unset(COMPILE_FLAGS_Linux)
list( APPEND  COMPILE_FLAGS_Linux   "" )

list( APPEND  COMPILE_FLAGS ${COMPILE_FLAGS_${CMAKE_SYSTEM_NAME}} )

list(APPEND   COMPILE_FLAGS ${TEST_DEFS} )

## Linker flags
unset(LINKER_FLAGS)
list( APPEND  LINKER_FLAGS  "" )

unset(LINKER_FLAGS_XCORE)
list( APPEND  LINKER_FLAGS_XCORE  "-target=${XCORE_TARGET}"     )
list( APPEND  LINKER_FLAGS_XCORE  "-report"                     )
list( APPEND  LINKER_FLAGS_XCORE  "-fcmdline-buffer-bytes=1024" )
list( APPEND  LINKER_FLAGS_XCORE  "${XSCOPE_CONFIG}"            )
list( APPEND  LINKER_FLAGS_XCORE  "-g"            )

unset(LINKER_FLAGS_Linux)
list( APPEND  LINKER_FLAGS_Linux  ""                            )

list( APPEND  LINKER_FLAGS ${LINKER_FLAGS_${CMAKE_SYSTEM_NAME}} )

add_executable( ${APP_NAME} ${SOURCES} )

target_link_libraries( ${APP_NAME} ${DEP_LIBS})


list( APPEND INCLUDES src 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/testing
    ${LIB_DSP_INCLUDES} 
    ${LIB_VAD_INCLUDES}
    ${LIB_AI_INCLUDES}
    ${XSCOPE_FILEIO_INCLUDES}
    )

target_include_directories( ${APP_NAME} PRIVATE ${INCLUDES} )


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