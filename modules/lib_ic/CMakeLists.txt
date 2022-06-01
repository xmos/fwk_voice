file( GLOB_RECURSE    LIB_IC_SOURCES       src/*.c )

## Create library target
add_library(avona_module_lib_ic STATIC)

target_sources(avona_module_lib_ic
    PRIVATE
        ${LIB_IC_SOURCES}
)

target_include_directories(avona_module_lib_ic
    PUBLIC
        api
)

target_compile_options(avona_module_lib_ic
    PRIVATE
        "-Os"
)

target_link_libraries(avona_module_lib_ic
    PUBLIC
        core::xs3_math
        avona::aec
        avona::vnr::features
        avona::vnr::inference
)

## Create an alias
add_library(avona::ic ALIAS avona_module_lib_ic)
