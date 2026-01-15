Using `lib_vnr` with `xcommon_cmake`
====================================

The example `CMakeLists.txt` in this test demonstrates the use of `lib_vnr` with `xcommon_cmake`.
However, it relies on `fwk_voice` being fetched, as the test is inside the repository.
To fetch `fwk_voice` and use `lib_vnr`, the `CMakeLists.txt` should look something like this:

.. code-block:: cmake

  cmake_minimum_required(VERSION 3.21)
  include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
  project(app_vnr)

  set(APP_HW_TARGET XK-EVK-XU316)
  set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../)
  set(XMOS_DEP_DIR_lib_voice ${XMOS_SANDBOX_DIR}/fwk_voice)
  set(APP_DEPENDENT_MODULES "lib_voice")
  if(NOT EXISTS ${XMOS_SANDBOX_DIR}/fwk_voice)
  include(FetchContent)
  FetchContent_Declare(
    fwk_voice
    GIT_REPOSITORY git@github.com:xmos/fwk_voice
    GIT_TAG develop
    SOURCE_DIR ${XMOS_SANDBOX_DIR}/fwk_voice
  )
  FetchContent_Populate(fwk_voice)
  endif()

  XMOS_REGISTER_APP()

In addition, before running the `cmake` command to fetch `fwk_voice`.
The user will have to set up the python environment and install `xmos-ai-tools`.
Both `python` and `xmos-ai-tools` have to have the same version as specified in the `requirements.txt`.
For example:

.. code-block:: console

  # create venv with python 3.10 first
  pip install xmos-ai-tools==1.3.1
