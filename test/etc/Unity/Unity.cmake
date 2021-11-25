

function( add_unity_to_target  target_name )

set( UNITY_PATH ${CMAKE_BINARY_DIR}/deps/Unity )

unset(UNITY_SOURCES)
list( APPEND UNITY_SOURCES
    "${UNITY_PATH}/src/unity.c"
    "${UNITY_PATH}/extras/fixture/src/unity_fixture.c"
)

target_sources( ${target_name} PRIVATE ${UNITY_SOURCES} )

target_include_directories( ${target_name} PRIVATE 
    ${UNITY_PATH}/src
    ${UNITY_PATH}/extras/fixture/src/
    ${UNITY_PATH}/extras/memory/src/
)


# If "CONFIG" is given as an optional argument, tell Unity to include "unity_config.h"
# (user should make sure unity_config.h is in the include path)
if (CONFIG IN_LIST ARGN)
  target_compile_definitions( ${target_name} PRIVATE "UNITY_INCLUDE_CONFIG_H")
endif()

if( XCORE )
  list( APPEND  UNITY_FLAGS  -Wno-xcore-fptrgroup )
endif()

string(REPLACE ";" " " UNITY_FLAGS "${UNITY_FLAGS}")
set_source_files_properties( ${UNITY_SOURCES} PROPERTIES  COMPILE_FLAGS  "${UNITY_FLAGS}" )

endfunction()