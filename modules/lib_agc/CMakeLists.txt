
## Create library target
add_library(fwk_voice_module_lib_agc STATIC)

target_sources(fwk_voice_module_lib_agc
    PRIVATE
        src/agc_impl.c
)

target_include_directories(fwk_voice_module_lib_agc
    PUBLIC
        api
)

target_compile_options(fwk_voice_module_lib_agc
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_agc
    PUBLIC
        lib_xcore_math
)


## Create an alias
add_library(fwk_voice::agc ALIAS fwk_voice_module_lib_agc)
