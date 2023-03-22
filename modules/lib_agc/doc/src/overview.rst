.. _agc_overview:

AGC Overview
************

The ``lib_agc`` library provides an API to implement Automatic Gain Control within
an application. The goal of the AGC algorithm is to provide consistent output
levels for voice audio.

The gain control can adapt to maintain the amplitude of the peak of the frame
within an upper and lower bound configured for the AGC instance. When used in an
application with a Voice to Noise Ratio estimator (VNR), the AGC will adapt only when
voice activity is detected, so that speech in the input signal is amplified
above other sounds.

The AGC also has a Loss Control feature which can be used when the application
has an Acoustic Echo Canceller (AEC). This feature uses data from the AEC to
adjust the gain applied to reduce residual echoes by attenuating the audio when
near-end speech is not present.

The AGC takes as input a frame of data from an audio channel. This could be the
microphone input or the output of another module in the application.

Gain control is performed on a frame-by-frame basis. Each frame consists of 15ms
of data, which is 240 samples at 16kHz input sampling frequency. Input data is
expected to be in a fixed-point 32-bit 1.31 format.

Before processing any frames, the application must configure and initialise the
AGC instance by calling ``agc_init()``. Then for each frame,
``agc_process_frame()`` will update the AGC instance's internal state and produce
the output frame by applying the AGC algorithm to the input frame.

The gain values in this module for AGC gain and Loss Control gain are
multiplicative factors that are applied to scale the input frame. Therefore, a
fixed gain value of 1.0 (without loss control) will create no change to the input.

If multiple channels need to be processed by the application, or multiple outputs
are required, an independent instance of the AGC must be run for each channel.
