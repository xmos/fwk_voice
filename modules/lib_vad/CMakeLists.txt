
file(GLOB_RECURSE LIB_VAD_SOURCES src/*.c)
add_library(fwk_voice_module_lib_vad STATIC)

target_sources(fwk_voice_module_lib_vad
    PRIVATE
        ${LIB_VAD_SOURCES}
)

target_include_directories(fwk_voice_module_lib_vad
    PUBLIC
        api
        src
)

target_compile_options(fwk_voice_module_lib_vad
    PRIVATE
        "-Os"
)

target_link_libraries(fwk_voice_module_lib_vad
    PUBLIC
        core::xs3_math
)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_definitions(fwk_voice_module_lib_vad
        PRIVATE
            VAD_MODULE_X86_BUILD=1)
endif()

## Create an alias
add_library(fwk_voice::vad ALIAS fwk_voice_module_lib_vad)
