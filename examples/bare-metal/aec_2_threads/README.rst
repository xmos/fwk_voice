
aec_2_threads
=============

This example demonstrates how AEC functions are called on 2 threads to process data through the AEC stage of a pipeline.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the AEC stage frame by frame.
The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input.
Echo cancelled version of the mic input is generated as the AEC output and written to the output.wav file.


Building
********

Run the following commands in the fwk_voice/build folder to build the firmware for the XCORE-AI-EXPLORER board as a target:

.. tab:: Linux and Mac

    .. code-block:: console
    
        cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake ..
        make fwk_voice_example_bare_metal_aec_2_thread

.. tab:: Windows

    .. code-block:: console

        # make sure you have the patch command available
        cmake -G "Ninja" --toolchain  ../xmos_cmake_toolchain/xs3a.cmake ..
        ninja fwk_voice_example_bare_metal_aec_2_thread

.. include :: ../../../doc/install_ninja_rst.inc


Running
*******

From the fwk_voice/build folder run:

.. tab:: Linux and Mac

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd ../examples/bare-metal/aec_2_thread
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_thread/bin/fwk_voice_example_bare_metal_aec_2_thread.xe --input ../shared_src/test_streams/aec_example_input.wav

.. tab:: Windows

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd fwk_voice_deps/xscope_fileio/host
        cmake -G "Ninja" .
        ninja
        cd ../../../../examples/bare-metal/aec_2_thread
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_thread/bin/fwk_voice_example_bare_metal_aec_2_thread.xe --input ../shared_src/test_streams/aec_example_input.wav

Output
******

The output file output.wav is generated in the `fwk_voice/examples/bare-metal/aec_2_threads` directory. The input file
input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the echo cancelled
output against the microphone input.
