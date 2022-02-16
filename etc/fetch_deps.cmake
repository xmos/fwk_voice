## fetch dependencies
include(FetchContent)

if ( ${BUILD_TESTS} )
  FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.5.2
    GIT_SHALLOW    TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/Unity
  )
  FetchContent_Populate(unity)

endif()
