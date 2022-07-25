
vad
===

This example demonstrates how the VAD function is called on a single thread to generate an estimate of the voice probability for the input audio stream.

In this example, a 32-bit, 1 channel wav file input.wav is read and passed to the VAD function frame by frame.
The VAD is pre-configured and pre-trained to give a high probability estimate in the presence of voice and a low probability estimate in the
absence of voice, even if there is other non-voice activity in the signal. 

Building
********

After configuring the CMake project, the following commands can be used from the
`fwk_voice/examples/bare-metal/vad` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make fwk_voice_example_bare_metal_vad
    cd ../examples/bare-metal/vad
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/vad/bin/fwk_voice_example_bare_metal_vad.xe --input input.wav


Output
------

The output from the VAD is printed to the console as an 8 bit unsigned integer which shows the estimated
VAD for each frame (240 samples) of audio passed. An excerpt from the output is show below.

::

    [DEVICE] frame: 159 vad: 0
    [DEVICE] frame: 160 vad: 0
    [DEVICE] frame: 161 vad: 0
    [DEVICE] frame: 162 vad: 1
    [DEVICE] frame: 163 vad: 145
    [DEVICE] frame: 164 vad: 226
    [DEVICE] frame: 165 vad: 246
    [DEVICE] frame: 166 vad: 97
    [DEVICE] frame: 167 vad: 236
    [DEVICE] frame: 168 vad: 255
    [DEVICE] frame: 169 vad: 255
    [DEVICE] frame: 170 vad: 249
    [DEVICE] frame: 171 vad: 253
    [DEVICE] frame: 172 vad: 43
    [DEVICE] frame: 173 vad: 146
    [DEVICE] frame: 174 vad: 249
    [DEVICE] frame: 175 vad: 229
    [DEVICE] frame: 176 vad: 61
    [DEVICE] frame: 177 vad: 7
    [DEVICE] frame: 178 vad: 1

The [DEVICE] prefix shows that the device (xCORE) has printed the line. The line contains the frame number and the VAD output estimate.