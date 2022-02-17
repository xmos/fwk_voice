#file( GLOB_RECURSE    LIB_AEC_SOURCES    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c )

#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*")

add_library( lib_aec  STATIC src/aec_impl.c src/aec_l2_impl.c src/aec_priv_impl.c )

target_include_directories( lib_aec PUBLIC api )

target_link_libraries(lib_aec PRIVATE sdk::lib_xs3_math)

set_target_properties( lib_aec PROPERTIES   PREFIX      ""
                                            SUFFIX      ".a")
