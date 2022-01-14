## Source files
file( GLOB_RECURSE    LIB_AEC_C_SOURCES       src/*.c )
file( GLOB_RECURSE    LIB_AEC_ASM_SOURCES     src/arch/xcore/*.S   )

## set LIB_AEC_INCLUDES & LIB_AEC_SOURCES
set( LIB_AEC_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"           )

unset(LIB_AEC_SOURCES_XCORE)
list( APPEND  LIB_AEC_SOURCES_XCORE   ${LIB_AEC_ASM_SOURCES} )

unset(LIB_AEC_SOURCES)
list( APPEND  LIB_AEC_SOURCES   ${LIB_AEC_C_SOURCES}    )
list( APPEND  LIB_AEC_SOURCES   ${LIB_AEC_SOURCES_${CMAKE_SYSTEM_NAME}}    )

## cmake doesn't recognize .S files as assembly by default
set_source_files_properties( ${LIB_XS3_MATH_ASM_SOURCES} PROPERTIES LANGUAGE ASM )

