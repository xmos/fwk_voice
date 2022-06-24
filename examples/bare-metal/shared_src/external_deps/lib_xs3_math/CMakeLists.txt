
set(XS3_MATH_PATH ${CMAKE_BINARY_DIR}/fwk_voice_deps)
## Source files
file(GLOB_RECURSE LIB_C_SOURCES
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/bfp/*.c
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/vect/*.c
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/scalar/*.c
)
file(GLOB_RECURSE LIB_CXX_SOURCES ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/*.cc)
file(GLOB_RECURSE LIB_XC_SOURCES  ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/*.xc)
file(GLOB_RECURSE LIB_ASM_SOURCES ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/*.S )
list(APPEND LIB_C_SOURCES    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/etc/xs3_fft_lut/xs3_fft_lut.c)

## cmake doesn't recognize .S files as assembly by default
set_source_files_properties(LIB_ASM_SOURCES PROPERTIES LANGUAGE ASM)

## Assume all asm is XS3A for now
set(XCORE_XS3A_SOURCES ${LIB_ASM_SOURCES})
list(APPEND XCORE_XS3A_SOURCES ${LIB_XC_SOURCES})

## Set any local library compile options
set(LIB_COMPILE_FLAGS "-Os")
if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
        ## Needed for CFFI tests
        list(APPEND LIB_COMPILE_FLAGS "-fPIC")
    endif()
endif()


## Includes files
set(LIB_PUBLIC_INCLUDES
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/api
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/api/bfp
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/api/scalar
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/api/vect
    ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/etc/xs3_fft_lut
)
set(LIB_PRIVATE_INCLUDES lib_xs3_math/lib_xs3_math/src)

## Gather library sources
set(LIB_PUBLIC_SOURCES   "")
set(LIB_PRIVATE_SOURCES  ${LIB_C_SOURCES} ${LIB_CXX_SOURCES})

## Append platform specific sources
if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
    list(APPEND LIB_PRIVATE_SOURCES ${XCORE_XS3A_SOURCES})
    list(APPEND LIB_PUBLIC_INCLUDES ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/arch/xcore)
else()
    file(GLOB_RECURSE X86_SOURCES ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/arch/ref/*.c)
    list(APPEND LIB_PRIVATE_SOURCES ${X86_SOURCES})
    list(APPEND LIB_PUBLIC_INCLUDES ${XS3_MATH_PATH}/lib_xs3_math/lib_xs3_math/src/arch/ref)
endif()

## Create library target
add_library(fwk_voice_deps_lib_xs3_math STATIC)
target_sources(fwk_voice_deps_lib_xs3_math
    PUBLIC
        ${LIB_PUBLIC_SOURCES}
    PRIVATE
        ${LIB_PRIVATE_SOURCES}
)
target_include_directories(fwk_voice_deps_lib_xs3_math
    PUBLIC
        ${LIB_PUBLIC_INCLUDES}
    PRIVATE
        ${LIB_PRIVATE_INCLUDES}
)
target_compile_options(fwk_voice_deps_lib_xs3_math
    PRIVATE
        ${LIB_COMPILE_FLAGS}
)
#target_link_libraries(fwk_voice_deps_lib_xs3_math
#    PUBLIC
#        sdk::legacy_compat
#)

## Create an alias
add_library(core::xs3_math ALIAS fwk_voice_deps_lib_xs3_math)
