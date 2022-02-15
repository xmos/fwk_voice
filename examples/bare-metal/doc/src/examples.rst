.. _examples:

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


Example App: agc
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



Example App: ic
===============


This example demonstrates how the IC functions are called to process data through the IC stage of a voice pipeline.

A 32-bit, 2 channel wav file input.wav is read and processed through the IC stage frame by frame. The input file consistes of 2 channels of
mic input consisting of a `Alexa` utterances with a point noise source consisting of pop music. The signal and noise sources in input.wav
come from different spatial locations.

The interference cancelled version of the mic input is generated as the IC output and written to the output.wav file. In this example, a VAD
is not used and so the VAD signal is set to 0 to indicate that voice is not present, meaning adaption will occur. In a practical system, the
VAD probability would increase during the utterances to ensure the IC does not adapt to the voice and cause it to be attenuated. The test
file has only a few short voice utterances and so the example works and demonstrates the IC operation.

Building
********

After configuring the CMake project, the following commands can be used from the
`sw_avona/examples/bare-metal/ic` directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:

::
    
    cd ../../../build
    make ic_example
    cd ../examples/bare-metal/ic
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/ic/bin/ic_example.xe

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/ic` directory. When viewing output.wav in a visual audio tool, such as Audacity, you can see a stark differnce between the channels emitted. Channel 0 is the IC output and is suitable for increasing the SNR in automatic speech recognition (ASR) applications. Channel 1 is the `simple beamformed` (average of mic 0 and mic 1 inputs) which may be preferable in comms (human to human) applications. The logic for channel 1 is contained in the ``ic_test_task.c`` file and is not part of the IC library.s

.. image:: ic_output.png
    :alt: Comparision between the IC output and simple beamformed output

You can see the drastic reduction of the amplitude of the music noise source in channel 0 after just just a few seconds whilst the voice signal source is preserved. By zooming in to the start of the waveform, you can also see the 212 sample (180 y_delay + 32 framing delay) latency through the IC, when compared with the averaged output of channel 1.



Example App: pipeline_single_threaded
=====================================

This example demonstrates how the audio processing stages are put together in a pipeline

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudpeaker delay offsets at start up in order to maximise AEC performance.

AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
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

NS is configured in the same way for both ASR and Comms. The NS stage generated noise suppressed version of the AGC output and then sent to AGC. 

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
    make pipeline_multi_threaded
    cd ../examples/bare-metal/pipeline_multi_threaded
    python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/pipeline_multi_threaded.xe --input ../shared_src/test_streams/pipeline_example_input.wav

Output
------

The output file output.wav is generated in the `sw_avona/examples/bare-metal/pipeline_multi_threaded` directory. The
input file input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the
pipeline output against the microphone input.
