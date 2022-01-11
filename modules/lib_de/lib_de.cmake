## Source files
file( GLOB_RECURSE    LIB_DE_C_SOURCES       src/*.c )

## set LIB_DE_INCLUDES & LIB_DE_SOURCES
set( LIB_DE_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"           )

unset(LIB_DE_SOURCES)
list( APPEND  LIB_DE_SOURCES   ${LIB_DE_C_SOURCES}    )

