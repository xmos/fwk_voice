
## Target name
set( LIB_NAME   lib_adec  )

## Export lib directory as variable
set( ADEC_PATH ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE )

include(lib_adec.cmake)

## Set library output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)

## Deliverable is a static library
add_library( ${LIB_NAME}  STATIC      ${LIB_ADEC_SOURCES} )

target_include_directories( ${LIB_NAME} PUBLIC
    ${LIB_ADEC_INCLUDES}
    )

target_link_libraries(${LIB_NAME} lib_xs3_math)

set_target_properties( ${LIB_NAME} PROPERTIES   PREFIX      ""
                                                OUTPUT_NAME ${LIB_NAME}
                                                SUFFIX      ".a"        )

target_compile_options( ${LIB_NAME} PRIVATE ${COMPILE_FLAGS} )

target_compile_options( ${LIB_NAME} PRIVATE "$<$<CONFIG:DEBUG>:-DDEBUGG=1>" )

