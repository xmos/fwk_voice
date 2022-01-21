.. _examples:

####################
Example Applications
####################

This example demonstrates how AGC functions are called on a single thread to process data through the AGC stage of a pipeline.


Building Examples
=================

After configuring the CMake project (with the ``BUILD_EXAMPLES`` enabled), all the examples can
be built by using the ``make`` command within the build directory. Individual examples can be built
using ``make EXAMPLE_NAME``, where ``EXAMPLE_NAME`` is the example to build.


Example App: agc_example
========================

A 32-bit single channel wav file input.wav is read and processed through the AGC stage frame by frame.
AGC is configured for ...

The gained version of the input is generated as the AGC output and written to the single channel output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the `sw_avona/examples/bare-metal/agc`
directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make agc_example
    cd ../examples/bare-metal/agc
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/agc/bin/agc_example.xe

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/agc` directory. View output.wav and input.wav 
in Audacity to compare the gained output against the input recording.
