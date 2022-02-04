.. _noise_suppression_overview:

NS Overview
~~~~~~~~~~~

The ``lib_noise_suppression`` library provides an API to implement Noise 
Suppression within an application. 

The noise suppressor estimates the porbability of speech presence and dynamically 
adapts it's coefficients to estimate the loise levels to subtract from the input. 
The filter will automatically reset it's noise estimations every 10 frames.

The NS takes as input a frame of data from an audio channel. This could be the
microphone input or the output of another module in the application.

Noise Suppression is performed on a frame-by-frame basis. Each frame consists of 
15ms of data, which is 240 samples at 16kHz input sampling frequency. Input data is
expected to be in a fixed-point 32-bit 1.31 format.

Before processing any frames, the application must configure and initialise the
NS instance by calling ``sup_init()``. Then for each frame,
``sup_process_frame()`` will update the NS instance's internal state and produce
the output frame by applying the NS algorithm to the input frame.

If multiple channels need to be processed by the application, or multiple outputs
are required, an independent instance of the NS must be run for each channel.
