## Source files
file( GLOB_RECURSE    LIB_VAD_C_SOURCES       src/*.c )

set( LIB_VAD_INCLUDES  ${CMAKE_CURRENT_LIST_DIR}/api ${CMAKE_CURRENT_LIST_DIR}/src )

unset(LIB_VAD_SOURCES_XCORE)
list( APPEND  LIB_VAD_SOURCES_XCORE  ${LIB_VAD_ASM_SOURCES} )

unset(LIB_VAD_SOURCES)
list( APPEND  LIB_VAD_SOURCES   ${LIB_VAD_C_SOURCES})
list( APPEND  LIB_VAD_SOURCES   ${LIB_VAD_SOURCES_${CMAKE_SYSTEM_NAME}}    )


## cmake doesn't recognize .S files as assembly by default
set_source_files_properties( ${LIB_XS3_MATH_ASM_SOURCES} PROPERTIES LANGUAGE ASM )
