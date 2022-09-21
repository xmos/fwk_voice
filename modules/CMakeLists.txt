if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
        #This is needed for the CFFI tests
        list(APPEND MODULE_COMPILE_FLAGS -fPIC)
        add_compile_options( ${MODULE_COMPILE_FLAGS} )
    endif()
endif()

add_subdirectory( lib_vnr )

add_subdirectory( lib_ns )

add_subdirectory( lib_aec )

add_subdirectory( lib_ic )

add_subdirectory( lib_agc )

add_subdirectory( lib_adec )

