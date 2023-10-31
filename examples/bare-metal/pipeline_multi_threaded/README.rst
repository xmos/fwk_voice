
pipeline_multi_threaded
=======================

This example demonstrates how the audio processing stages are put together in a pipeline where stages are run in
parallel on separate hardware threads.

In it, a 32-bit, 4 channel wav file input.wav is read and processed through the pipeline stages frame by frame. The
example currently demonstrates a pipeline having AEC, IC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudspeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC. The VNR is introduced
to give the IC and the AGC information about the speech presence in a frame.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. This example calls AEC functions using 2 threads to process a frame through the AEC stage.
The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
This example supports a maximum of 150ms of delay correction, in either direction, between the reference and microphone input.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the
IC.

The IC only processes a two channel input. It will use the second channel as the reference to the first to output one channel of interference cancelled output.
In this manner, it tries to cancel the room noise. However, to avoid cancelling the wanted signal, it only adapts in the absence of voice.
Hence the VNR is called to calculate the voice to noise ratio estimation. The output of the VNR will allow IC to modulate the rate
at which it adapts it's coefficients. The output of the IC is copied to the second channel as well.

The NS is a single channel API, so two instances of NS should be initialised for 2 channel processing. The NS is configured the same way 
for both channels. It will try to predict the background noise and cancel it from the frame before passing it to AGC.

The AGC is configured for ASR engine suitable gain control on both channels. The
output of AGC stage is the pipeline output which is written into a 2 channel output wav file. The AGC also takes the output
of the VNR to control when to adapt. This avoids noise being amplified during the absence of voice.

In total, the audio processing stages consume 5 hardware threads; 2 for AEC stage, 1 for IC and VNR, 1 for NS stage and 1 for AGC stage.
Note that it is possible to run the full pipeline in as little as two 75MHz threads if required using one thread for stage 1 and
a second thread for all remaining blocks.

The input file input.wav has a total of 4 channels and should have bit depth of 32b. Due to the microphone inputs being very low amplitude,
16b data would result in severe quantisation of the data. The first 2 channels in the 4 channel file are the mic inputs followed by 2 channels 
of reference input. Output is written to the output.wav file consisting of 2 channels.

Building
********

Run the following commands in the fwk_voice/build folder to build the firmware for the XCORE-AI-EXPLORER board as a target:

.. tab:: Linux and Mac

    .. code-block:: console
    
        cmake --toolchain ../xmos_cmake_toolchain/xs3a.cmake ..
        make fwk_voice_example_bare_metal_pipeline_multi_thread

.. tab:: Windows

    .. code-block:: console

        # make sure you have the patch command available
        cmake -G "Ninja" --toolchain  ../xmos_cmake_toolchain/xs3a.cmake ..
        ninja fwk_voice_example_bare_metal_pipeline_multi_thread

.. include :: ../../../doc/install_ninja_rst.inc

Running
*******

From the fwk_voice/build folder run:

.. tab:: Linux and Mac

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd ../examples/bare-metal/pipeline_multi_threaded
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_bare_metal_pipeline_multi_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav

.. tab:: Windows

    .. code-block:: console

        pip install -e fwk_voice_deps/xscope_fileio
        cd fwk_voice_deps/xscope_fileio/host
        cmake -G "Ninja" .
        ninja
        cd ../../../../examples/bare-metal/pipeline_multi_threaded
        python ../shared_src/python/run_xcoreai.py ../../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_bare_metal_pipeline_multi_thread.xe --input ../shared_src/test_streams/pipeline_example_input.wav

Output
******

The output file output.wav is generated in the `fwk_voice/examples/bare-metal/pipeline_multi_threaded` directory. The
input file input.wav is also present in the same directory. View output.wav and input.wav in Audacity to compare the
pipeline output against the microphone input.
