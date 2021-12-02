
## Target name
set( LIB_NAME   lib_meta_data  )

## Export lib directory as variable
set( META_DATA_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE )

include(meta_data.cmake)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/libs/)

## Set library output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

## Deliverable is a static library
add_library( ${LIB_NAME}  STATIC      ${META_DATA_SOURCES} )

## Build lib_xs3_math
#set(LIB_XS3_MATH_PATH ${CMAKE_BINARY_DIR}/deps/xcore_sdk/modules/lib_xs3_math/lib_xs3_math)  ## For fetching dependencies using cmake

target_include_directories( ${LIB_NAME} PUBLIC   
    ${META_DATA_INCLUDES}
    ${LIB_XS3_MATH_PATH}/api
    ${LIB_XS3_MATH_PATH}/api/bfp )

target_link_libraries(${LIB_NAME} lib_xs3_math)

set_target_properties( ${LIB_NAME} PROPERTIES   PREFIX      ""
                                                OUTPUT_NAME ${LIB_NAME}
                                                SUFFIX      ".a"        )

target_compile_options( ${LIB_NAME} PRIVATE ${COMPILE_FLAGS} )

target_compile_options( ${LIB_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUGG=1>" )
