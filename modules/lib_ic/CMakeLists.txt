file( GLOB_RECURSE    LIB_IC_SOURCES       src/*.c )
add_library( lib_ic  STATIC ${LIB_IC_SOURCES} )

set( LIB_AEC_PRIV_INCLUDES  "../lib_aec/src"  )
target_include_directories( lib_ic PUBLIC api ${LIB_AEC_PRIV_INCLUDES} )

target_link_libraries(lib_ic lib_aec lib_xs3_math)

set_target_properties( lib_ic PROPERTIES   PREFIX      ""
                                           SUFFIX      ".a")
