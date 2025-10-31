.. _agc_overview:

AGC Overview
************

The ``lib_agc`` library provides an API to implement Automatic Gain Control within
an application. The AGC algorithm can dynamically adapt the audio gain,
or apply a fixed gain such that voice content maintains a desired output
level. The AGC uses an internal Voice Activity Detector to normalise
voice content and avoid amplifying noise sources and applies a soft
limiter to avoid clipping on the output. The design is based on standard
modern AGC techniques as detailed in '*Acoustic Echo and Noise
Control',* by Hansler and Schmidt.

The gain control can adapt to maintain the amplitude of the peak of the frame
within an upper and lower bound configured for the AGC instance. When used in an
application with a Voice to Noise Ratio estimator (VNR), the AGC will adapt only when
voice activity is detected, so that speech in the input signal is amplified
above other sounds.

The Loss Control process improves the subjective audio quality by
attenuating any residual echo of the reference far-end audio. It is
designed to be used on the communications channel. In cases where there
is both far-end echo and near-end audio then the attenuation is reduced,
allowing listeners to interrupt each other. The Loss Control relies on
the Automatic Echo Canceller (AEC) to classify and attenuate residual far-end
echo.

An optional soft clipping stage is applied at the end of the AGC to
avoid hard clipping of the output signal during sudden loud sounds.

AGC Application
***************

The AGC takes as input a frame of data from an audio channel. This could be the
microphone input or the output of another module in the application.

Gain control is performed on a frame-by-frame basis. Each frame consists of 15ms
of data, which is 240 samples at 16kHz input sampling frequency. Input data is
expected to be in a fixed-point 32-bit 1.31 format.

Before processing any frames, the application must configure and initialise the
AGC instance by calling ``agc_init()``. Several parameter sets are provided in 
`agc_profiles.h` which can be used to configure the AGC for different
applications. Details on the profiles and key parameters are provided in :ref:`agc_profiles`.

After initialisation, ``agc_process_frame()`` should be called for each frame.
This will update the AGC instance's internal state and produce
the output frame by applying the AGC algorithm to the input frame.

The gain values in this module for AGC gain and Loss Control gain are
multiplicative factors that are applied to scale the input frame. Therefore, a
fixed gain value of 1.0 (without loss control) will create no change to the input.

If multiple channels need to be processed by the application, or multiple outputs
are required, an independent instance of the AGC must be run for each channel.


AGC Logic
*********

The internal logic of the AGC algorithm is represented in the flow chart
shown in :numref:`agc_logic`. This diagram illustrates the main decision points and processing
steps performed for each input frame. It shows how the AGC determines
whether to adapt the gain based on voice activity, applies peak and
threshold checks, manages loss control, and optionally performs soft
clipping. 

.. _agc_logic:

.. figure:: ./images/agc_logic.drawio.svg
    :align: center

    AGC Logic Flow Chart

The logic of the loss control process is shown in the flow chart
in :numref:`loss_control_logic`. This diagram illustrates how the loss control
estimates the state and applies the appropriate attenuation based on
the presence of far-end echo and near-end audio. It is only used when the
loss control feature is enabled in the AGC configuration.

.. _loss_control_logic:

.. figure:: ./images/loss_control_logic.drawio.svg
    :align: center

    Loss Control Logic Flow Chart

