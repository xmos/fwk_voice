
file( GLOB_RECURSE    LIB_ADEC_SOURCES       src/*.c )

## Create library target
add_library(avona_module_lib_adec STATIC)

target_sources(avona_module_lib_adec
    PRIVATE
        ${LIB_ADEC_SOURCES}
)

target_include_directories(avona_module_lib_adec
    PUBLIC
        api
)

target_compile_options(avona_module_lib_adec
    PRIVATE
        "-Os"
)

target_link_libraries(avona_module_lib_adec
    PUBLIC
        sdk::lib_xs3_math
        avona::aec
)

## Create an alias
add_library(avona::adec ALIAS avona_module_lib_adec)
