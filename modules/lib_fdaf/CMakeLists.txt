
## Create library target
add_library(fwk_voice_module_lib_fdaf STATIC)

target_sources(fwk_voice_module_lib_fdaf
    PRIVATE
        src/fdaf_filter.c
        src/fdaf_control.c
        src/fdaf_l2_filter.c
)

target_include_directories(fwk_voice_module_lib_fdaf
    PUBLIC
        api
        src
)

target_compile_options(fwk_voice_module_lib_fdaf
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_fdaf
    PUBLIC
        core::xs3_math
)

## Create an alias
add_library(fwk_voice::fdaf ALIAS fwk_voice_module_lib_fdaf)
