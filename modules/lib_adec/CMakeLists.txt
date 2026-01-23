
file( GLOB_RECURSE    LIB_ADEC_SOURCES       src/*.c )

## Create library target
add_library(fwk_voice_module_adec INTERFACE)

target_sources(fwk_voice_module_adec
    INTERFACE
        ${LIB_ADEC_SOURCES}
)

target_include_directories(fwk_voice_module_adec
    INTERFACE
        api
)

target_compile_options(fwk_voice_module_adec
    INTERFACE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_adec
    INTERFACE
        fwk_voice::aec
)

## Create an alias
add_library(fwk_voice::adec ALIAS fwk_voice_module_adec)
