
add_library(avona_test_shared_test_utils INTERFACE)

file( GLOB_RECURSE SHARED_SOURCES *.c)
target_sources(avona_test_shared_test_utils INTERFACE ${SHARED_SOURCES})

target_include_directories(avona_test_shared_test_utils INTERFACE pseudo_rand testing)

add_library(avona::test::shared::test_utils ALIAS avona_test_shared_test_utils) 

#################

add_library(avona_test_shared_unity INTERFACE)

file( GLOB UNITY_SOURCES
    ${UNITY_PATH}/src/*.c)
target_sources(avona_test_shared_unity INTERFACE ${UNITY_SOURCES})

target_include_directories(avona_test_shared_unity
    INTERFACE
        ${UNITY_PATH}/src 
        ${UNITY_PATH}/extras/fixture/src
        ${UNITY_PATH}/extras/memory/src)

target_compile_definitions(avona_test_shared_unity
    INTERFACE
        -DUNITY_SUPPORT_64)

add_library(avona::test::shared::unity ALIAS avona_test_shared_unity)

#################
add_subdirectory(legacy_compat)

if(CMAKE_SYSTEM_NAME STREQUAL XCORE_XS3A)
    add_subdirectory(lib_dsp)
endif()
