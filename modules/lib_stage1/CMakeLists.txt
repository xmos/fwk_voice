
## Create library target
add_library(fwk_voice_module_stage1 INTERFACE)

target_sources(fwk_voice_module_stage1
    INTERFACE
        src/delay_buffer.c
        src/stage1.c
)

target_include_directories(fwk_voice_module_stage1
    INTERFACE
        api
        src
)

target_compile_options(fwk_voice_module_stage1
    INTERFACE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_stage1
    INTERFACE
        fwk_voice::adec
)

## Create an alias
add_library(fwk_voice::stage1 ALIAS fwk_voice_module_stage1)
