if( NOT ${Python3_FOUND} )
  message(FATAL_ERROR "Python3 not found for running . ") 
endif()

#copy conftest.py in the build directory since pytest_collect_file only collects tests from the directory tree where conftest.py is present
configure_file( conftest.py conftest.py COPYONLY ) 

add_executable(avona_ns_unit_tests)
file( GLOB_RECURSE SOURCES_C  src/*.c )
target_sources(avona_ns_unit_tests
    PRIVATE
        ${SOURCES_C}
        ${UNITY_PATH}/extras/fixture/src/unity_fixture.c)
target_include_directories(avona_ns_unit_tests
    PRIVATE
        src)
target_link_libraries(avona_ns_unit_tests
    PUBLIC
        avona::ns
        avona::test::shared::test_utils
        avona::test::shared::unity)
if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    target_compile_options(avona_ns_unit_tests
        PRIVATE 
            "-DUNITY_SUPPORT_64"
            "-Wno-xcore-fptrgroup"
            "-report")

    target_link_options(avona_ns_unit_tests
        PRIVATE
            "-target=${XCORE_TARGET}")
else()
    target_link_libraries(avona_ns_unit_tests m)
endif()
