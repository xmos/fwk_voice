###########
QUICK START
###########

Requirements
------------

* XTC Tools 15.2.1 or higher
* CMake 3.20 or higher
* Python 3.10 or higher

.. include :: ../install_ninja_rst.inc


Building
--------

The following instructions show how to build the Voice Framework and run one of the example applications. This
procedure is currently supported on MacOS, Linux and Windows.

#. Enter the clone of the Voice Framework and initialise submodules

   .. code-block:: console

     cd fwk_voice
     git submodule update --init --recursive

#. Create a build directory

   .. code-block:: console

     mkdir build
     cd build

#. Run cmake to setup the build environment for the XMOS toolchain

   - On Linux and Mac

      .. code-block:: console

        cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake ..

   - On Windows

      .. code-block:: console

        # make sure you have the patch command available
        cmake -G "Ninja" --toolchain  ../xmos_cmake_toolchain/xs3a.cmake ..

#. Running make will then build the Voice Framework libraries and example applications

   - On Linux and Mac

      .. code-block:: console

         make fwk_voice_example_bare_metal_aec_1_thread

   - On Windows

      .. code-block:: console

         ninja fwk_voice_example_bare_metal_aec_1_thread

#. Install dependencies

   - On Linux and Mac

      .. code-block:: console

         pip install -e build/fwk_voice_deps/xscope_fileio/

   - On Windows

      .. code-block:: console

         pip install -e fwk_voice_deps/xscope_fileio
         cd fwk_voice_deps/xscope_fileio/host
         cmake -G "Ninja" .
         ninja
         cd ../../../

.. raw:: pdf

   PageBreak oneColumn

6. Run the single-threaded AEC example

   .. code-block:: console

      cd ../examples/bare-metal/aec_1_thread
      python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/fwk_voice_example_bare_metal_aec_1_thread.xe --input ../shared_src/test_streams/aec_example_input.wav

   See ``Example Applications`` section in the User Guide for full details about the examples.
