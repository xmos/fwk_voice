
## Defines for tests

## The unit test apps

add_subdirectory( test_wav_aec )
if( XCORE )
    #add_subdirectory( test_aec_enhancements )
    #add_subdirectory( test_delay_estimator )
    #add_subdirectory( test_aec_spec )
    #add_subdirectory( test_aec_profile )
    #add_subdirectory( aec_unit_tests )
endif()
