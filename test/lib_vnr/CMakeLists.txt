
add_subdirectory(vnr_unit_tests)

add_subdirectory(test_wav_vnr)

if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(profile_memory)
endif()
