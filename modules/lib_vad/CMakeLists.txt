
file(GLOB_RECURSE LIB_VAD_SOURCES src/*.c)
add_library(avona_module_lib_vad STATIC)

target_sources(avona_module_lib_vad
    PRIVATE
        ${LIB_VAD_SOURCES}
)

target_include_directories(avona_module_lib_vad
    PUBLIC
        api
        src
)

target_link_libraries(avona_module_lib_vad
    PUBLIC
        sdk::lib_xs3_math
)

## Create an alias
add_library(avona::vad ALIAS avona_module_lib_vad)

