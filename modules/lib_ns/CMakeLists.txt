
## Create library target
add_library(avona_module_lib_ns STATIC)

target_sources(avona_module_lib_ns
    PRIVATE
        src/ns_impl.c
        src/ns_priv_impl.c
)

target_include_directories(avona_module_lib_ns
    PUBLIC
        api
        src
)

target_link_libraries(avona_module_lib_ns
    PUBLIC
        sdk::lib_xs3_math
)

## Create an alias
add_library(avona::ns ALIAS avona_module_lib_ns)
