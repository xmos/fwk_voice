

include(CMakeDependentOption)



#### BUILD OPTIONS ####  

## If enabled, the unit tests will be added as build targets
set(BUILD_TESTS    OFF CACHE BOOL "Include tests as CMake targets." )
set(BUILD_EXAMPLES ON CACHE BOOL "Include examples as CMake targets." )
set( TEST_WAV_AEC_BUILD_CONFIG "2 2 2 10 5" CACHE STRING "AEC build configuration for test_wav_aec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( TEST_AEC_ENHANCEMENTS_BUILD_CONFIG "2 2 2 10 5" CACHE STRING "AEC build configuration for test_aec_enhancements in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( TEST_DELAY_ESTIMATOR_BUILD_CONFIG "2 2 2 10 5" CACHE STRING "AEC build configuration for test_delay_estimator in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( TEST_AEC_SPEC_BUILD_CONFIG "2 1 1 20 10" CACHE STRING "AEC build configuration for test_aec_spec in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( TEST_AEC_PROFILE_BUILD_CONFIG "2 2 2 10 5" "1 2 2 10 5" CACHE STRING "AEC build configurations for test_aec_profile in <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( AEC_UNIT_TESTS_BUILD_CONFIG "2 2 2 10 5" CACHE STRING "AEC build configuration for aec_unit_tests in <threads> <ychannels> <xchannels> <num_main_phases> <num_shadow_phases> format" )
set( AEC_UNIT_TESTS_SPEEDUP_FACTOR "1" CACHE STRING "AEC unit tests speedup factor." )

#### PRINT OPTIONS ####

message(STATUS "BUILD_TESTS:    ${BUILD_TESTS}")

