
## Defines for tests

## The unit test apps

add_subdirectory( test_wav_ic )
if( XCORE )
    add_subdirectory( ic_unit_tests )
    add_subdirectory( test_ic_profile )
    add_subdirectory( test_ic_spec )
    add_subdirectory( characterise_xc_py )
endif()
