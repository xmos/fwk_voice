#####################################
fwk_voice: Voice processing framework
#####################################

********
Overview
********

``fwk_voice`` is a collection of DSP components used to build a front-end voice processing pipeline.

At its core, the framework provides high-performance audio processing algorithms that are combined
into a configurable pipeline. The pipeline takes input from a pair of microphones and applies
a sequence of signal processing stages to extract a clean voice signal from complex acoustic environments.
An optional reference signal from a host system can be provided to enable Acoustic Echo Cancellation (AEC),
removing echo from the microphone signal.

The pipeline produces two output streams: one optimized for Automatic Speech Recognition (ASR)
systems and another suitable for voice communications.

``fwk_voice`` includes a flexible audio routing infrastructure and supports a range of
digital inputs and outputs, allowing it to be integrated into a wide variety of system configurations.
The pipeline can be configured at startup and adjusted during operation via a set configuration parameters.
All source code is provided, enabling full customization and the integration of additional audio processing algorithms.

***************************
Voice Processing Components
***************************

.. toctree::
   :maxdepth: 1

   audio_processing/aec/index
   audio_processing/ns/index
   audio_processing/agc/index
   audio_processing/adec/index
   audio_processing/stage1/index
   audio_processing/ic/index
   audio_processing/vnr/index


********
Examples
********

Various example applications are provided along side the ``fwk_voice`` that demonstrate basic usage.
These are located in the ``examples`` directory.

Requirements
============

To build or run any examples the user is expected to have the following:

* XTC Tools 15.3.1
* CMake 3.20 or higher
* Python 3.10 or higher

The python is required since library depends on the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_
This is required for the ``cmake`` to configure the project,
so the user must create a python environment and install ``xmos-ai-tools``, before running ``cmake``.

AEC example
===========

The AEC example demonstrates the use of API to run a frame of data through the AEC.
It also shows how to configure AEC to run on one or two threads.
For that purpose, this example is built with 2 build configs ``_1th`` and ``_2th``.

Building
--------

To build the example, run the following from the root of the repository:

.. code-block:: console

   git submodule update --init --recursive
   pip install -r requirements.txt
   cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
   xmake -C build fwk_voice_example_aec_1th
   xmake -C build fwk_voice_example_aec_2th


Running
-------

To run the example, run the following from the root of the repository:

.. code-block:: console

   xrun --io build/examples/aec/bin/fwk_voice_example_aec_1th.xe
   xrun --io build/examples/aec/bin/fwk_voice_example_aec_2th.xe

Output
------

Upon execution, the example will print "frame done" when the AEC has processed a frame.

VNR example
===========

The VNR example demonstrates the use of API to run a frame of data through the VNR
and get the estimation of how much voice is present in it.

Building
--------

To build the example, run the following from the root of the repository:

.. code-block:: console

   git submodule update --init --recursive
   pip install -r requirements.txt
   cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
   xmake -C build fwk_voice_example_vnr

Running
-------

To run the example, run the following from the root of the repository:

.. code-block:: console

   xrun --io build/examples/vnr/bin/fwk_voice_example_vnr.xe

Output
------

Upon execution, the example will use the pseudo-random generator to get an input data.
This data will be run through the VNR and the score will be printed in the terminal.
It outputs a number between 0 and 1, 1 being the strongest voice with respect to noise
and 0 being the lowest voice compared to noise ratio.
The pseudo-random data is not representative of a real signal,
so the VNR scores in this example tend to be zero.

Pipeline example
================

This example demonstrates how to put together a voice processing pipeline.
The pipeline will have AEC, IC, NS and AGC stages. It also demonstrates the use of ADEC module to
do a one time estimation and correction for possible reference and loudspeaker delay offsets at start up in order to
maximise AEC performance.  ADEC processing happens on the same thread as the AEC. The VNR is introduced
to give the IC and the AGC information about the speech presence in a frame.

There are two pipelines supported in this example: Standard Architecture and Alternating Architecture.
For that purpose, this example is built with 2 build configs ``_std_arch`` and ``_alt_arch``.

Standard Architecture
---------------------

In this pipeline form, all the modules are enabled and called sequentially.

The AEC is configured for 2 mic input channels, 2 reference input channels, 10 phase main filter and a 5 phase shadow
filter. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
The AEC stage generates the echo cancelled version of the mic input that is then sent for processing through the IC.

Alternating Architecture
------------------------

In this pipeline form, the AEC and the IC frame processing are selectively enabled and disabled based on the presence of reference input signal.
Acoustic Echo Cancellation is performed only if activity is detected on the reference input channels and disabled otherwise.
Interference Cancellation is performed only when AEC is disabled so in the absence of reference channel activity and disabled otherwise.

The AEC is configured for 1 mic input channel, 2 reference input channels, 15 phase main filter and a 5 phase shadow
filter giving an extended tail length for highly reverberant environments. The AEC gets reconfigured as a 1 mic input channel, 1 reference input channel, 30 main filter phases and no shadow
filter, when ADEC goes in delay estimation mode. This allows it to measure the room delay. During this process, the AEC
output is ignored and the mic input is directly sent to output. Once the new delay has been measured and the delay correction is
applied, the AEC gets configured back to its original configuration and starts adapting and cancellation.
In the absence of activity on the reference channels, when the AEC is disabled, the mic input is copied directly to the output of the AEC.

Common
------

If enabled, the IC only processes a two channel input. It will use the second channel as the reference to the first to output one channel of interference cancelled output.
In this manner, it tries to cancel the room noise. However, to avoid cancelling the wanted signal, it only adapts in the absence of voice.
Hence the VNR is called to calculate the voice to noise ratio estimation. The output of the VNR will allow IC to modulate the rate
at which it adapts its coefficients. The output of the IC is copied to the second channel as well.

The NS is a single channel API, so two instances of NS should be initialised for 2 channel processing. The NS is configured the same way
for both the channels. It will try to predict the background noise and cancel it from the frame before passing it to AGC.

The AGC is configured for ASR engine suitable gain control on both the channels. The
output of the AGC stage is the pipeline output. The AGC also takes the output
of the VNR to adapt its coefficients. This avoids noise being amplified during the absence of voice.

Building
--------

To build the example, run the following from the root of the repository:

.. code-block:: console

   git submodule update --init --recursive
   pip install -r requirements.txt
   cmake -G "Unix Makefiles" -B build --toolchain xmos_cmake_toolchain/xs3a.cmake
   xmake -C build fwk_voice_example_pipeline_std_arch
   xmake -C build fwk_voice_example_pipeline_alt_arch


Running
-------

To run the example, run the following from the root of the repository:

.. code-block:: console

   xrun --io build/examples/pipeline/bin/fwk_voice_example_pipeline_std_arch.xe
   xrun --io build/examples/pipeline/bin/fwk_voice_example_pipeline_alt_arch.xe

Output
------

Upon execution, the example will print "frame done" when the pipeline has processed a frame.

