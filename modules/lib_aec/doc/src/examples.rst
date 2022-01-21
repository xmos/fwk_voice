.. _examples:

####################
Example Applications
####################

There are two examples offered to demonstrate use of the ``lib_aec`` APIs through actual, simple code examples.


Building Examples
=================

After configuring the CMake project (with the ``BUILD_EXAMPLES`` enabled), all the examples can
be built by using the ``make`` command within the build directory.  Individual examples can be built
using ``make EXAMPLE_NAME``, where ``EXAMPLE_NAME`` is the example to build. 


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

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/aec_1_thread` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make aec_1_thread_example
    cd ../examples/bare-metal/aec_1_thread
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/aec_1_thread_example.xe

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/aec_1_thread` directory. View output.wav and input.wav 
in Audacity to compare the echo cancelled output against the microphone input.


Example App: aec_2_threads
==========================

This example demonstrates how AEC functions are called on 2 threads to process data through the AEC stage of a pipeline.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the AEC stage frame by frame.
AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input.
Echo cancelled version of the mic input is generated as the AEC output and written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/aec_2_threads` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make aec_2_threads_example
    cd ../examples/bare-metal/aec_2_threads
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_threads/bin/aec_2_threads_example.xe

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/aec_2_threads` directory. View output.wav and input.wav 
in Audacity to compare the echo cancelled output against the microphone input.

