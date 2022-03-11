
pipeline_single_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC, IC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudspeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC. The VAD is also demonstrated here
to give the IC and the AGC information about the speech presence in a frame.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
This example supports a maximum of 150ms of delay correction, in either direction, between the reference and microphone input.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the
IC to form the ASR channel. The comms channel will be produced by averaging both AEC stage outputs, acting a simple beamformer.

The IC only processes a two channel input. It will use the second channel as the reference to the first to output interference cancelled output.
In this manner, it tries to cancel the room noise. However, to avoid cancelling the wanted signal, it only adapts in the absence of voice.
Hence the VAD is called to calculate the probability of the voice activity. The output of the VAD will allow IC to modulate the rate
at which it adapts it's coefficients.

The NS is a single channel API, so two instances of NS should be initialised for 2 channel processing. The NS is configured the same way 
for both ASR and Comms. It will try to predict the background noise and cancel it from the frame before passing it to AGC.

Before passing the Comms channel through the AGC it needs to be filtered. It's being passed through the 100Hz High-Pass filter,
which will cancel any frequency bins that are below 100Hz, since there shouldn't be any voice there.

The AGC is configured for ASR engine suitable gain control on channel 0 and Comms suitable gain control on channel 1. The
output of the AGC stage is the pipeline output which is written into a 2 channel output wav file. The AGC also takes the output
of the VAD to adapt it's coefficients.

The pipeline is run on a single thread. To run the pipeline on a single xcore-AI thread a minimum thread speed of 140MHz is recommended, for 
a typical pipeline configuration.

The input file input.wav has a total of 4 channels and should have bit depth of 32b. Due to the microphone inputs being very low amplitude,
16b data would result in severe quantisation of the data. The first 2 channels in the 4 channel file are the mic inputs followed by 2 channels 
of reference input. Output is written to the output.wav file consisting of 2 channels; channel 0 is ASR and channel 1 is comms.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/pipeline_single_threaded` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make avona_example_bare_metal_pipeline_single_thread
    cd ../examples/bare-metal/pipeline_single_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav
