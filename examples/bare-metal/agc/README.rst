
agc
===

This example demonstrates how AGC functions are called on a single thread to process data through the AGC stage of
a pipeline. A single AGC instance is run using the profile that is tuned for communication with a human listener.

Since this example application only demonstrates the AGC module, without a VNR or an AEC, adaption based on voice
activity and the loss control feature are both disabled.

The input is a single channel, 32-bit wav file, which is read and processed through the AGC frame-by-frame.


Building
********

Run the following commands in the fwk_voice/build folder to build the firmware for the XCORE-AI-EXPLORER board as a target:

.. tab:: Linux and Mac

    .. code-block:: console
    
        cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake ..
        make fwk_voice_example_bare_metal_agc

.. tab:: Windows

    .. code-block:: console

        # make sure you have the patch command available
        cmake -G "Ninja" --toolchain  ../xmos_cmake_toolchain/xs3a.cmake ..
        ninja fwk_voice_example_bare_metal_agc

.. include :: ../../../doc/install_ninja_rst.inc


Running
*******

From the fwk_voice/build folder run:

.. tab:: Linux and Mac

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd ../examples/bare-metal/agc
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/agc/bin/fwk_voice_example_bare_metal_agc.xe --input ../shared_src/test_streams/agc_example_input.wav

.. tab:: Windows

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd fwk_voice_deps/xscope_fileio/host
        cmake -G "Ninja" .
        ninja
        cd ../../../../examples/bare-metal/agc
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/agc/bin/fwk_voice_example_bare_metal_agc.xe --input ../shared_src/test_streams/agc_example_input.wav

Output
******

The output file output.wav is generated in the `fwk_voice/examples/bare-metal/agc` directory. The provided
input `agc_example_input.wav` is low-volume white-noise and the effect of the AGC can be heard in the output
by listening to the two wav files.
