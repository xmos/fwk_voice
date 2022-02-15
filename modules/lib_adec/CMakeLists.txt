file( GLOB_RECURSE    LIB_ADEC_SOURCES       src/*.c )
add_library( lib_adec  STATIC ${LIB_ADEC_SOURCES} )

target_include_directories( lib_adec PUBLIC api )

target_link_libraries(lib_adec lib_xs3_math lib_aec )

set_target_properties( lib_adec PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
