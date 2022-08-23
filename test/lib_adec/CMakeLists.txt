
add_subdirectory( test_wav_adec )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( test_adec )
    add_subdirectory( test_adec_profile )
    add_subdirectory( test_delay_estimator )
    add_subdirectory( de_unit_tests )
    add_subdirectory( test_adec_startup )
endif()
