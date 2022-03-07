
## Platform-specific C/C++ compile flags

if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
    set(XCORE_TARGET "XCORE-AI-EXPLORER")
else()
    list(APPEND   COMPILE_FLAGS  -DX86_BUILD=1)
endif()
add_subdirectory( bare-metal )
