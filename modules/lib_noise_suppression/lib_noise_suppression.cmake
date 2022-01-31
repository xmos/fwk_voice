## Source files
file( GLOB_RECURSE    NS_C_SOURCES       src/*.c )

## set NS_INCLUDES & NS_SOURCES
set( NS_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"   "${CMAKE_CURRENT_LIST_DIR}/src" )

unset(NS_SOURCES)
list( APPEND  NS_SOURCES   ${NS_C_SOURCES}    )

list( APPEND  NS_SOURCES  ${NS_SOURCES_${CMAKE_SYSTEM_NAME}}        )

## cmake doesn't recognize .S files as assembly by default
set_source_files_properties( ${LIB_XS3_MATH_ASM_SOURCES} PROPERTIES LANGUAGE ASM )
