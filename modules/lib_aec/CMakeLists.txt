file( GLOB_RECURSE    LIB_AEC_SOURCES       src/*.c )

#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*")

add_library( lib_aec  STATIC ${LIB_AEC_SOURCES} )

target_include_directories( lib_aec PUBLIC api )

target_link_libraries(lib_aec lib_xs3_math)

set_target_properties( lib_aec PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
