

include(CMakeDependentOption)


## If enabled, the unit tests will be added as build targets
set(BUILD_TESTS    ON CACHE BOOL "Include unit tests as CMake targets." )


#### PRINT OPTIONS ####

message(STATUS "BUILD_TESTS:    ${BUILD_TESTS}")