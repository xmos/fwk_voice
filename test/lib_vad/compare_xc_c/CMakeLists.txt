## App name
set( APP_NAME  compare_xc_c )

# Auto-generate schedule and top level config files
if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

#copy conftest.py in the build directory since pytest_collect_file only collects tests from the directory tree where conftest.py is present
configure_file( conftest.py conftest.py COPYONLY ) 

## Depends on libraries
list( APPEND  DEP_LIBS_XCORE  ""  )
list( APPEND  DEP_LIBS_Linux  m   )

list( APPEND  DEP_LIBS        
    lib_xs3_math
    lib_vad
    test_shared
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} 
)

list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )

## Sources

# If "CONFIG" is given as an optional argument, tell Unity to include "unity_config.h"
# (user should make sure unity_config.h is in the include path)
if (  CONFIG IN_LIST ARGN  )
  target_compile_definitions( ${target_name} PRIVATE "UNITY_INCLUDE_CONFIG_H")
endif()

string(REPLACE ";" " " UNITY_FLAGS "${UNITY_FLAGS}")
set_source_files_properties( ${UNITY_SOURCES} PROPERTIES  COMPILE_FLAGS  "${UNITY_FLAGS}" )

file( GLOB_RECURSE SOURCES_C  src/*.c )
file( GLOB_RECURSE  SOURCES_RAND  ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand/*.c )
file( GLOB_RECURSE  SOURCES_TEST  ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/testing/*.c )

set(LIB_DSP_PATH ${XCORE_SDK_PATH}/modules/lib_dsp/lib_dsp )

file( GLOB_RECURSE LIB_DSP_SOURCES_C  ${LIB_DSP_PATH}/src/*.c )
file( GLOB_RECURSE LIB_DSP_SOURCES_XC  ${LIB_DSP_PATH}/src/*.xc )
file( GLOB_RECURSE LIB_DSP_SOURCES_ASM  ${LIB_DSP_PATH}/src/*.S )
set( LIB_DSP_SOURCES  ${LIB_DSP_SOURCES_C} ${LIB_DSP_SOURCES_XC} ${LIB_DSP_SOURCES_ASM})

list( APPEND  SOURCES   ${SOURCES_C}  ${SOURCES_RAND}  ${SOURCES_TEST} ${LIB_DSP_SOURCES} )
list( APPEND  SOURCES   ${SOURCES_${CMAKE_SYSTEM_NAME}} )
list( APPEND  SOURCES ${UNITY_SOURCES} )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG ../../etc/config.xscope ABSOLUTE)

## Compile flags
unset(COMPILE_FLAGS)
list( APPEND  COMPILE_FLAGS   -Wno-unused-variable )

unset(COMPILE_FLAGS_XCORE)
list( APPEND  COMPILE_FLAGS_XCORE   "")

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
    ${CMAKE_SOURCE_DIR}/modules/lib_vad/api
    ${LIB_DSP_PATH}/api
    ${UNITY_PATH} 
    ${UNITY_PATH}/../extras/fixture/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/pseudo_rand
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/testing
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