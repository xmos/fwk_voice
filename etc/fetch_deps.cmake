## fetch dependencies
include(FetchContent)

if ( ${BUILD_TESTS} )
  FetchContent_Declare(
    xcore_sdk
    GIT_REPOSITORY      git@github.com:xmos/xcore_sdk
    GIT_TAG             origin/develop
    GIT_SUBMODULES      modules/lib_xs3_math modules/lib_dsp
    GIT_SHALLOW         TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/xcore_sdk
  )
  FetchContent_Populate(xcore_sdk)

  FetchContent_Declare(
    audio_test_tools
    GIT_REPOSITORY      git@github.com:xmos/audio_test_tools
    GIT_TAG             v4.5.1
    GIT_SHALLOW         TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/audio_test_tools
  )
  FetchContent_Populate(audio_test_tools)

  FetchContent_Declare(
    unity
    GIT_REPOSITORY      git@github.com:xmos/Unity
    GIT_TAG             origin/develop
    GIT_SHALLOW         TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/Unity
  )
  FetchContent_Populate(unity)

  FetchContent_Declare(
    xscope_fileio
    GIT_REPOSITORY      git@github.com:xmos/xscope_fileio
    GIT_TAG             v0.3.2
    GIT_SHALLOW         TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/xscope_fileio
  )
  FetchContent_Populate(xscope_fileio)
else()
  FetchContent_Declare(
    xcore_sdk
    GIT_REPOSITORY      git@github.com:xmos/xcore_sdk
    GIT_TAG             origin/develop
    GIT_SUBMODULES      modules/lib_xs3_math
    GIT_SHALLOW         TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR          ${CMAKE_BINARY_DIR}/deps/xcore_sdk
  )
  FetchContent_Populate(xcore_sdk)
<<<<<<< HEAD
endif()
=======
endif()
>>>>>>> c0ac639d25728453afd8b80a469d469972a0fdd0
