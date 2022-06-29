
## Create library target
add_library(avona_module_lib_ns STATIC)

target_sources(avona_module_lib_ns
    PRIVATE
        src/ns_impl.c
        src/ns_low_level.c
)

target_include_directories(avona_module_lib_ns
    PUBLIC
        api
        src
)

target_compile_options(avona_module_lib_ns
    PRIVATE
        -Os
        -g
)

target_link_libraries(avona_module_lib_ns
    PUBLIC
        core::xs3_math
)

## Create an alias
add_library(avona::ns ALIAS avona_module_lib_ns)
