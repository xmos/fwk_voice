
pipeline_single_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudpeaker delay offsets at start up in order to maximise AEC performance.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the
NS. AEC gets reconfigured for a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode while measuring and correcting delay offsets. During this process, AEC
output is ignored and the mic input is directly sent to output. Once the new delay is measured and delay correction is
applied, AEC gets configured back to its original configuration.
This example supports a maximum of 150ms of delay correction between the reference and microphone input.

The NS is configured in the same way for both ASR and Comms. The NS stage generates noise suppressed version of the AEC output
that is sent downstream for AGC processing. 

The AGC is configured for ASR engine suitable gain control on channel 0 and Comms suitable gain control on channel 1. The
output of the AGC stage is the pipeline output which is written into a 2 channel output wav file.

The pipeline is run on a single thread.

The input file input.wav has 2 channels of mic input followed by 2 channels of reference input. Output is written to the output.wav file.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/pipeline_single_threaded` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make avona_example_bare_metal_pipeline_single_thread
    cd ../examples/bare-metal/pipeline_single_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav
