
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

target_compile_options(avona_module_lib_vad
    PRIVATE
        -Os
        -g
)

target_link_libraries(avona_module_lib_vad
    PUBLIC
        core::xs3_math
)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_definitions(avona_module_lib_vad
        PRIVATE
            VAD_MODULE_X86_BUILD=1)
endif()

## Create an alias
add_library(avona::vad ALIAS avona_module_lib_vad)
