if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    #Both of these tests compare against old XC libs which are difficult to make work under x86 builds, and not needed, so skip
    add_subdirectory( vad_unit_tests )
    add_subdirectory( compare_xc_c )
    add_subdirectory( test_vad_profile )
endif()
