set(XCORE_MATH_PATH ${CMAKE_BINARY_DIR}/fwk_voice_deps)

add_subdirectory(${XCORE_MATH_PATH}/lib_xcore_math/lib_xcore_math ${XCORE_MATH_PATH}/build)

if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
        ## Needed for CFFI tests
        target_compile_options(lib_xcore_math PRIVATE "-fPIC")
    endif()
endif()