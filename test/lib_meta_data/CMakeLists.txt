## App name
set( APP_NAME  test_meta_data )

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
    lib_meta_data
    test_shared
    ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} 
)

list( APPEND  DEP_LIBS ${DEP_LIBS_${CMAKE_SYSTEM_NAME}} )

## Sources

# Unity
set( UNITY_PATH ${DEPS_ROOT}/Unity/src )
file( GLOB UNITY_SOURCES  ${UNITY_PATH}/*.c 
                          ${UNITY_PATH}/../extras/fixture/src/unity_fixture.c)

# If "CONFIG" is given as an optional argument, tell Unity to include "unity_config.h"
# (user should make sure unity_config.h is in the include path)
if (CONFIG IN_LIST ARGN)
  target_compile_definitions( ${target_name} PRIVATE "UNITY_INCLUDE_CONFIG_H")
endif()

string(REPLACE ";" " " UNITY_FLAGS "${UNITY_FLAGS}")
set_source_files_properties( ${UNITY_SOURCES} PROPERTIES  COMPILE_FLAGS  "${UNITY_FLAGS}" )

file( GLOB_RECURSE SOURCES_C  src/*.c )
list( APPEND  SOURCES         ${SOURCES_C}     )
list( APPEND  SOURCES   ${SOURCES_${CMAKE_SYSTEM_NAME}} )
list( APPEND  SOURCES ${UNITY_SOURCES} )

# set( XSCOPE_CONFIG config.xscope )
get_filename_component(XSCOPE_CONFIG ../etc/config.xscope ABSOLUTE)

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
${CMAKE_SOURCE_DIR}/modules/lib_meta_data/lib_meta_data/api
${CMAKE_SOURCE_DIR}/test/meta_data/shared/pseudo_rand
${CMAKE_SOURCE_DIR}/test/meta_data/shared/testing
${UNITY_PATH} 
${UNITY_PATH}/../extras/fixture/src)

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

