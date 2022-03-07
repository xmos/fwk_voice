
vad
===

This example demonstrates how the VAD function is called on a single thread to generate an estimate of the voice probability for the input audio stream.

In this example, a 32-bit, 1 channel wav file input.wav is read and passed to the VAD function frame by frame.
The VAD is pre-configured and pre-trained to give a high probability estimate in the presence of voice and a low probability estimate in the
absence of voice, even if there is other non-voice activity in the signal. 

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/vad` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make avona_example_bare_metal_vad
    cd ../examples/bare-metal/vad
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/vad/bin/avona_example_bare_metal_vad.xe --input input.wav


Output
------

The output from the VAD is printed to the console as an 8 bit unsigned integer which shows the estimated
VAD for each frame (240 samples) of audio passed.
