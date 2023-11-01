
ic
==

This example demonstrates how the IC functions are called to process data through the IC stage of a voice pipeline.

A 32-bit, 2 channel wav file input.wav is read and processed through the IC stage frame by frame. The input file consists of 2 channels of
mic input consisting of a `Alexa` utterances with a point noise source consisting of pop music. The signal and noise sources in input.wav
come from different spatial locations.

The interference cancelled version of the mic input is generated as the IC output and written to the output.wav file. In this example, a VNR
is not used and so the VNR signal is set to 0 to indicate that voice is not present, meaning adaption will occur. In a practical system, the
VNR voice to noise ratio would increase during the utterances to ensure the IC does not adapt to the voice and cause it to be attenuated. The test
file has only a few short voice utterances and so the example works and demonstrates the IC operation.


Building
********

Run the following commands in the fwk_voice/build folder to build the firmware for the XCORE-AI-EXPLORER board as a target:

.. tab:: Linux and Mac

    .. code-block:: console
    
        cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake ..
        make fwk_voice_example_bare_metal_ic

.. tab:: Windows

    .. code-block:: console

        # make sure you have the patch command available
        cmake -G "Ninja" --toolchain  ../xmos_cmake_toolchain/xs3a.cmake ..
        ninja fwk_voice_example_bare_metal_ic

.. include :: ../../../doc/install_ninja_rst.inc


Running
*******

From the fwk_voice/build folder run:

.. tab:: Linux and Mac

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd ../examples/bare-metal/ic
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/ic/bin/fwk_voice_example_bare_metal_ic.xe

.. tab:: Windows

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd fwk_voice_deps/xscope_fileio/host
        cmake -G "Ninja" .
        ninja
        cd ../../../../examples/bare-metal/ic
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/ic/bin/fwk_voice_example_bare_metal_ic.xe

Output
******

The output file output.wav is generated in the `fwk_voice/examples/bare-metal/ic` directory. When viewing output.wav in a visual audio tool, such as Audacity, you can see a stark difference between the channels emitted. Channel 0 is the IC output and is suitable for increasing the SNR in automatic speech recognition (ASR) applications. Channel 1 is the `simple beamformed` (average of mic 0 and mic 1 inputs) which may be preferable in comms (human to human) applications. The logic for channel 1 is contained in the ``ic_test_task.c`` file and is not part of the IC library.s

.. image:: ic_output.png
    :alt: Comparision between the IC output and simple beamformed output

You can see the drastic reduction of the amplitude of the music noise source in channel 0 after just just a few seconds whilst the voice signal source is preserved. By zooming in to the start of the waveform, you can also see the 212 sample (180 y_delay + 32 framing delay) latency through the IC, when compared with the averaged output of channel 1.
