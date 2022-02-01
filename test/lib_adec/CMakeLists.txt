
    add_subdirectory( test_adec )
if( XCORE )
    add_subdirectory( test_wav_adec )
    add_subdirectory( test_adec_profile )
    add_subdirectory( test_delay_estimator )
    add_subdirectory( de_unit_tests )
endif()
