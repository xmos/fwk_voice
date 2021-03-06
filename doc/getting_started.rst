###########
QUICK START
###########

Requirements
------------

* XTC Tools 15.0.6 or higher
* A clone of the `xcore_sdk <https://github.com/xmos/xcore_sdk/>`_, with its submodules initialised
* CMake 3.20 or higher
* Python 3.7 or higher


Building
--------

The following instructions show how to build Avona and run one of the example applications. This
procedure is currently supported on MacOS and Linux only.

#. Enter the clone of Avona and initialise submodules
     .. code-block:: console

       cd sw_avona
       git submodule update --init --recursive

#. Create a build directory
     .. code-block:: console

       mkdir build
       cd build

#. Run cmake to setup the build environment for the XMOS toolchain
     .. code-block:: console

       cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../xmos_cmake_toolchain/xs3a.cmake

#. Running make will then build the Avona libraries and example applications
     .. code-block:: console

       make

#. Install dependencies
     .. code-block:: console

       cd ../examples/bare-metal/aec_1_thread
       pip install -e ../shared_src/xscope_fileio

#. Run the single-threaded AEC example
     .. code-block:: console

       python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/avona_example_bare_metal_aec_1_thread.xe --input ../shared_src/test_streams/aec_example_input.wav

See :ref:`examples` for full details about the example applications.
