file( GLOB_RECURSE    LIB_MELSPECTROGRAM_SOURCES       src/*.c )

## Create library target
add_library(fwk_voice_module_lib_melspectrogram STATIC)

target_sources(fwk_voice_module_lib_melspectrogram
    PRIVATE
        ${LIB_MELSPECTROGRAM_SOURCES}
)

target_include_directories(fwk_voice_module_lib_melspectrogram
    PUBLIC
        api
)

target_compile_options(fwk_voice_module_lib_melspectrogram
    PRIVATE
        -Os
        -g
)

target_link_libraries(fwk_voice_module_lib_melspectrogram
    PUBLIC
        lib_xcore_math
)

## Create an alias
add_library(fwk_voice::melspectrogram ALIAS fwk_voice_module_lib_melspectrogram)
