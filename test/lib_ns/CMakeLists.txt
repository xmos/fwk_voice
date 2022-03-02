if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    #add_subdirectory( ns_priv_unit_tests )
    add_subdirectory( ns_unit_tests )
    add_subdirectory( test_wav_ns )
    add_subdirectory( test_ns_profile )
endif()

