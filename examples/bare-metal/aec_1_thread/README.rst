Example App: aec_1_thread
=========================

This example demonstrates how AEC functions are called on a single thread to process data through the AEC stage of a pipeline.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the AEC stage frame by frame.
AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow 
filter.
The input file input.wav has 2 channels of mic input followed by 2 channels of reference input.
Echo cancelled version of the mic input is generated as the AEC output and written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the current directory to build and run this
example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make aec_1_thread_example
    cd ../examples/bare-metal/aec_1_thread
    python run_xcoreai.py
