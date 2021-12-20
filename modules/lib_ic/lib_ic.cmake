## Source files
file( GLOB_RECURSE    LIB_IC_C_SOURCES       src/*.c )

## set LIB_IC_INCLUDES & LIB_IC_SOURCES
set( LIB_IC_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"           )

unset(LIB_IC_SOURCES)
list( APPEND  LIB_IC_SOURCES   ${LIB_IC_C_SOURCES}    )

