.. _overview:

IC Overview
===========

The Interference Canceller (IC) suppresses static noise from point sources such as cooker hoods, washing machines,
or radios for which there is no reference audio signal available. When the Voice Activity Detector (VAD) input
indicates the absence of voice, the IC adapts to remove noise from point sources in the environment. When the VAD 
signal indicates the presence of voice, the IC suspends adaptation which maintains suppression of the interfering 
noise sources previously adapted to.

The echo canceller is based on an AEC architecture and attempts to cancel one microphone signal from the other in
the absence of voice. In this way, it builds an estimate of the difference in transfer functions between the two
microphones for any present noise sources. Since the transfer function includes spatial information about the noise
sources, applying this filter to the mic input allows any signals originating from the noise source to be cancelled.

The IC uses an adaptive filter which continually adapts to the acoustic environment to accommodate changes in the room
created by events such as doors opening or closing and people moving about, or even the presence of intermittent 
noise sources such as kitchen applinces.
However it will hold the current transfer function in the presence of voice meaning it does not adapt to desired 
audio sources, which in this case is a person speaking.

The cancellation is performed on a frame by frame basis. Each frame is made of 15msec chunks of data, which is 240
new samples at 16kHz input sampling frequency, per input channel. This is combined with previous audio data to form
a 512 sample frame which allows for sufficient overlap for effective operation of the filter.

The first channel of input microphone data is referred to as `y` when in time domain and `Y` when in frequency
domain. The second channel of input microphone data is referred to as `x` when in time domain and `X` when in frequency
domain. The y signal is effectively used as the signal containing noise that needs to be cancelled and the x signal
is the refernce from which the transfer function is estimated and consequently the noise signal estimated before it
is subtracted from y.

In general throughout the code, names starting with lower case represent time domain and those beginning with
upper case represent frequency domain. For example `error` is the filter error and `Error` is the spectrum of
the filter error. The filter coefficient array referred to as `h_hat` in time domain and `H_hat` in frequency domain.

The filter has multiple phases. The term phases refers to the tail length of the filter. A filter with more phases or a
longer tail length will be able to model a more reverberant room response leading to better interference cancellation
but, as with all normalised LMS based architectures, will be slower to converge in the case of a transfer function change.

Before starting the IC processing the user must call ic_init() to initialise the IC. If the configuration parameters are
to be set to non-defaults please eith modify these after ic_init() or in the ic_defines.h file.
Once the IC is initialised, the library functions can be called in a order to perform interference cancellation on 
a frame by frame basis. Please see the `Example Application`. 

