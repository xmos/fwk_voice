
## Target name
set( LIB_NAME   lib_ic  )

## Export lib directory as variable
set( IC_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE )

include(lib_ic.cmake)
include(../lib_aec/lib_aec.cmake)

## Set library output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

## Deliverable is a static library
add_library( ${LIB_NAME}  STATIC      ${LIB_IC_SOURCES} )

## Build lib_xs3_math
set(LIB_XS3_MATH_PATH ${CMAKE_BINARY_DIR}/deps/xcore_sdk/modules/lib_xs3_math/lib_xs3_math)  ## For fetching dependencies using cmake


##Because these are not part of AEC public API
set( LIB_AEC_PRIV_INCLUDES  "../lib_aec/src"  )

target_include_directories( ${LIB_NAME} PUBLIC
    ${LIB_AEC_INCLUDES}
    ${LIB_AEC_PRIV_INCLUDES}
    ${LIB_IC_INCLUDES}
    ${LIB_XS3_MATH_PATH}/api 
    )

target_link_libraries(${LIB_NAME} lib_xs3_math lib_aec)

set_target_properties( ${LIB_NAME} PROPERTIES   PREFIX      ""
                                                OUTPUT_NAME ${LIB_NAME}
                                                SUFFIX      ".a"        )

target_compile_options( ${LIB_NAME} PRIVATE ${COMPILE_FLAGS} )

target_compile_options( ${LIB_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUGG=1>" )

