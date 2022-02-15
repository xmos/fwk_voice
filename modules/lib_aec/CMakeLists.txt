file( GLOB_RECURSE    LIB_AEC_SOURCES       src/*.c )
add_library( lib_aec  STATIC ${LIB_AEC_SOURCES} )

set( LIB_AEC_INCLUDES     "api" )
target_include_directories( lib_aec PUBLIC api )

target_link_libraries(lib_aec lib_xs3_math)

set_target_properties( lib_aec PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
