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
    make aec_1_thread
    cd ../examples/bare-metal/aec_1_thread
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_1_thread/bin/aec_1_thread.xe --input ../shared_src/test_streams/aec_example_input.wav


Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/aec_1_thread` directory. The input file
input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the echo cancelled
output against the microphone input.


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
    make aec_2_threads
    cd ../examples/bare-metal/aec_2_threads
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/aec_2_threads/bin/aec_2_threads.xe --input ../shared_src/test_streams/aec_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/aec_2_threads` directory. The input file
input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the echo cancelled
output against the microphone input.


Example App: pipeline_single_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC and AGC stages.

AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the
AGC.

AGC is configured for ASR engine suitable gain control on channel 0 and Comms suitable gain control on channel 1. The
output of AGC stage is the pipeline output which is written into a 2 channel output wav file.

The pipeline is run on a single thread.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input. Output is written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/pipeline_single_threaded` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make pipeline_single_threaded
    cd ../examples/bare-metal/pipeline_single_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_single_threaded/bin/pipeline_single_threaded.xe --input ../shared_src/test_streams/pipeline_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/pipeline_single_threaded` directory. The
input file input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the
pipeline output against the microphone input.

Example App: pipeline_multi_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline where stages are run in
parallel on separate hardware threads.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC and AGC stages.

AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. This example calls AEC functions using 2 threads to process a frame through the AEC stage. AEC stage generates
the echo cancelled version of the mic input that is then sent for processing through the AGC.

AGC is configured for ASR engine suitable gain control on channel 0 and Comms suitable gain control on channel 1. The
output of AGC stage is the pipeline output which is written into a 2 channel output wav file.

In total, the audio processing stages consume 3 hardware threads; 2 for AEC stage and 1 for AGC stage.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input. Output is written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/pipeline_multi_threaded` directory to build and run this example application using the
XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make pipeline_multi_threaded
    cd ../examples/bare-metal/pipeline_multi_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/pipeline_multi_threaded.xe --input ../shared_src/test_streams/pipeline_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/pipeline_multi_threaded` directory. The
input file input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the
pipeline output against the microphone input.

