
file( GLOB_RECURSE    LIB_ADEC_SOURCES       src/*.c )

## Create library target
add_library(fwk_voice_module_lib_adec STATIC)

target_sources(fwk_voice_module_lib_adec
    PRIVATE
        ${LIB_ADEC_SOURCES}
)

target_include_directories(fwk_voice_module_lib_adec
    PUBLIC
        api
)

target_compile_options(fwk_voice_module_lib_adec
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_adec
    PUBLIC
        lib_xcore_math
        fwk_voice::aec
)

## Create an alias
add_library(fwk_voice::adec ALIAS fwk_voice_module_lib_adec)
