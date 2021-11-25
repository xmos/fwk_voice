## Target name
set( LIB_NAME   meta_data  )

## Export lib directory as variable
set( META_DATA_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE )

include(meta_data.cmake)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/libs/)

## Set library output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

## Deliverable is a static library
add_library( ${LIB_NAME}  STATIC      ${META_DATA_SOURCES} )

target_include_directories( ${LIB_NAME} PUBLIC   
    ${META_DATA_INCLUDES}
    ${CMAKE_SOURCE_DIR}/../lib_xs3_math/lib_xs3_math/api
    ${CMAKE_SOURCE_DIR}/../lib_xs3_math/lib_xs3_math/api/bfp )

set_target_properties( ${LIB_NAME} PROPERTIES   PREFIX      ""
                                                OUTPUT_NAME ${LIB_NAME}
                                                SUFFIX      ".a"        )

target_compile_options( ${LIB_NAME} PRIVATE ${COMPILE_FLAGS} )

target_compile_options( ${LIB_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUGG=1>" )