
## Defines for tests

## The unit test apps

add_subdirectory( test_wav_ic )
add_subdirectory( test_calc_vnr_pred )
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( ic_unit_tests )
    add_subdirectory( test_ic_profile )
    add_subdirectory( test_ic_spec )
    add_subdirectory( test_bad_state )
    add_subdirectory( characterise_c_py )
endif()
