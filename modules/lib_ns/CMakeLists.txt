
## Create library target
add_library(fwk_voice_module_lib_ns STATIC)

target_sources(fwk_voice_module_lib_ns
    PRIVATE
        src/ns_impl.c
        src/ns_low_level.c
)

target_include_directories(fwk_voice_module_lib_ns
    PUBLIC
        api
        src
)

target_compile_options(fwk_voice_module_lib_ns
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_ns
    PUBLIC
        lib_xcore_math
)

## Create an alias
add_library(fwk_voice::ns ALIAS fwk_voice_module_lib_ns)
