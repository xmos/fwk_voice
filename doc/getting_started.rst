###########
QUICK START
###########

Requirements
------------

* XTC Tools 15.0.6 or higher
* CMake 3.20 or higher
* Python 3.7 or higher


Building
--------

The following instructions show how to build the Voice Framework and run one of the example applications. This
procedure is currently supported on MacOS and Linux only.

1. Enter the clone of the Voice Framework and initialise submodules

.. code-block:: console

  cd fwk_voice
  git submodule update --init --recursive

2. Create a build directory

.. code-block:: console

  mkdir build
  cd build

3. Run cmake to setup the build environment for the XMOS toolchain

.. tab:: Linux and Mac

  .. code-block:: console

    cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake

.. tab:: Windows

  .. code-block:: console

    # make sure you have the patch command available
    cmake -G "NMake Makefiles" -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake

As part of the cmake, some dependencies are fetched using CMake FetchContent. One of these dependencies, lib_tflite_micro has a patch applied to it as part of the FetchContent. This means, when trying to rerun the cmake in the same build directory, sometimes errors
related to not being able to apply a patch to an already patched library are seen. To get rid of these errors, add the -DFETCHCONTENT_UPDATES_DISCONNECTED=ON option to the cmake command line, which will disable the FetchContent if the content has been downloaded previously.

4. Running make will then build the Voice Framework libraries and example applications

.. tab:: Linux and Mac

  .. code-block:: console

    make fwk_voice_example_bare_metal_aec_1_thread

.. tab:: Windows

  .. code-block:: console

    nmake fwk_voice_example_bare_metal_aec_1_thread

5. Install dependencies

.. tab:: Linux and Mac

  .. code-block:: console

    pip install -e build/fwk_voice_deps/xscope_fileio/

.. tab:: Windows

  .. code-block:: console

    pip install -e fwk_voice_deps/xscope_fileio
    cd fwk_voice_deps/xscope_fileio/host
    cmake -G "NMake Makefiles" .
    nmake
    cd ../../../

6. Run the single-threaded AEC example

.. code-block:: console

  cd ../examples/bare-metal/aec_1_thread
  python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/fwk_voice_example_bare_metal_aec_1_thread.xe --input ../shared_src/test_streams/aec_example_input.wav

See :ref:`examples` for full details about the example applications.
