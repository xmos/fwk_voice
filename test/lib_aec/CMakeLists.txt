
## Defines for tests

## The unit test apps

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory( test_aec_enhancements )
    add_subdirectory( test_aec_spec )
    add_subdirectory( aec_unit_tests )
endif()
