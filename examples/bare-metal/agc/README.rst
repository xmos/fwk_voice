
agc
================

This example demonstrates how AGC functions are called on a single thread to process data through the AGC stage of
a pipeline. A single AGC instance is run using the profile that is tuned for communication with a human listener.

Since this example application only demonstrates the AGC module, without a VAD or an AEC, adaption based on voice
activity and the loss control feature are both disabled.

The input is a single channel, 32-bit wav file, which is read and processed through the AGC frame-by-frame.

Building
********

After configuring the CMake project, the following commands can be used from the `sw_avona/examples/bare-metal/agc`
directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::

    cd ../../../build
    make agc
    cd ../examples/bare-metal/agc
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/agc/bin/agc.xe --input ../shared_src/test_streams/agc_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/agc` directory. The provided
input `agc_example_input.wav` is low-volume white-noise and the effect of the AGC can be heard in the output
by listening to the two wav files.
