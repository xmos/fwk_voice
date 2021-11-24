## Source files
file( GLOB_RECURSE    LIB_AEC_C_SOURCES       src/*.c )

## set LIB_AEC_INCLUDES & LIB_AEC_SOURCES
set( LIB_AEC_INCLUDES     "${CMAKE_CURRENT_LIST_DIR}/api"           )

unset(LIB_AEC_SOURCES)
list( APPEND  LIB_AEC_SOURCES   ${LIB_AEC_C_SOURCES}    )

