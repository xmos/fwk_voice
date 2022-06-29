#file( GLOB_RECURSE    LIB_AEC_SOURCES    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c )

#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=*")

add_library(avona_module_lib_aec STATIC)

target_sources(avona_module_lib_aec
    PRIVATE
        src/aec_impl.c
        src/aec_l2_impl.c
        src/aec_priv_impl.c
)

target_include_directories(avona_module_lib_aec
    PUBLIC
        api
        src ## this is needed for the IC
)

target_compile_options(avona_module_lib_aec
    PRIVATE
        -Os
        -g
)

target_link_libraries(avona_module_lib_aec
    PUBLIC
        core::xs3_math
)

## Create an alias
add_library(avona::aec ALIAS avona_module_lib_aec)
