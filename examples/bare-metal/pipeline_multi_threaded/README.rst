
pipeline_multi_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline where stages are run in
parallel on separate hardware threads.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudpeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. This example calls AEC functions using 2 threads to process a frame through the AEC stage.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the
NS. AEC gets reconfigured for a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode while measuring and correcting delay offsets. During this process, AEC
output is ignored and the mic input is directly sent to output. Once the new delay is measured and delay correction is
applied, AEC gets configured back to its original configuration.
This example supports a maximum of 150ms of delay correction between the reference and microphone input.

The NS is configured in the same way for both ASR and Comms. The NS stage generated noise suppressed version of the AGC output and then sent to AGC. 

AGC is configured for ASR engine suitable gain control on channel 0 and Comms suitable gain control on channel 1. The
output of AGC stage is the pipeline output which is written into a 2 channel output wav file.

In total, the audio processing stages consume 4 hardware threads; 2 for AEC stage, 1 for NS stage and 1 for AGC stage.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input. Output is written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/pipeline_multi_threaded` directory to build and run this example application using the
XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make avona_example_bare_metal_pipeline_multi_thread
    cd ../examples/bare-metal/pipeline_multi_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/avona_example_bare_metal_pipeline_multi_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/pipeline_multi_threaded` directory. The
input file input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the
pipeline output against the microphone input.
