
## Create library target
add_library(avona_module_lib_agc STATIC)

target_sources(avona_module_lib_agc
    PRIVATE
        src/agc_impl.c
)

target_include_directories(avona_module_lib_agc
    PUBLIC
        api
)

target_link_libraries(avona_module_lib_agc
    PUBLIC
        sdk::lib_xs3_math
)


## Create an alias
add_library(avona::agc ALIAS avona_module_lib_agc)
