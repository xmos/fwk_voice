if(${CMAKE_SYSTEM_NAME} STREQUAL XCORE_XS3A)
    add_subdirectory(feature_extraction)
    add_subdirectory(inference)
endif()
