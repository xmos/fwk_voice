.. _aec_overview:

AEC Overview
************

The lib_aec library provides functions that can be put together to
perform Automatic Echo Cancellation on input microphone data by using
input reference data to model the echo characteristics of the room.

The echo canceller takes in one or more channels of microphone (mic)
input and one or more channels of reference input data. The mic input is
the input captured by the device microphones. Reference input is the
audio that is played out of the device speakers. The echo canceller uses
the reference input to model the room echo characteristics for each
mic-loudspeaker pair and outputs an echo cancelled version of the mic
input. AEC uses adaptive filters, one per mic-speaker pair to constantly
remove echo from the the mic input. The filters continually adapt to the
acoustic environment to accommodate changes in the room created by
events such as doors opening or closing and people moving about.

Echo cancellation is performed on a frame by frame basis. Each frame is
made of 15msec chunks of data, which is 240 samples at 16kHz input
sampling frequency, per input channel. For example, for a 2 mic channel
and 2 reference channel input configuration, an input frame is made of
2x240 samples of mic data and 2x240 samples of reference data. Input
data is expected to be in fixed point 32bit 1.31 format. Further, in
this example, there will be a total of 4 adaptive filters;
:math:`\hat{H}_{y0x0}`, :math:`\hat{H}_{y0x1}`, :math:`\hat{H}_{y1x0}`
and :math:`\hat{H}_{y1x1}`, monitoring the echo seen in mic channel 0
from reference channel 0 and 1 and echo seen in mic channel 1 from
reference channel 0 and 1.

Microphone data is referred to as :math:`y` when in time domain and
:math:`Y` when in frequency domain. In general throughout the code,
names starting with lower case represent time domain and those beginning
with upper case represent frequency domain. For example :math:`error` is
the filter error and :math:`Error` is the spectrum of the filter error.
Reference input is referred to as :math:`x` in time domain and :math:`X`
when in frequency domain. Filter is referred to as :math:`\hat{h}` in
time domain and :math:`\hat{H}` in frequency domain.

A filter has multiple phases. The term phases refers to the tail length
of the filter. A filter with more phases or a longer tail length will be
able to model a more reverberant room response leading to better echo
cancellation.

There are 2 types of adaptive filters used in the AEC. These are
referred to as main filter and shadow filter. The main filter as the
name suggests is the main filter that is used to generate the echo
cancelled output of the AEC. Shadow filter is a filter that used to
quickly detect and respond to changes in the room transfer function.
There is one main filter and one shadow filter per :math:`x`-:math:`y`
pair. Typically the main filter has more phases than the shadow filter.
Fewer phases in the shadow filter enable it to rapidly detect and
respond to changes while more phases in main filter lead to deeper
convergence and hence better echo cancellation at the AEC output.

Before starting AEC processing or every time there’s a configuration
change, the user needs to call aec_init() to initialise the echo
canceller for a desired configuration. Once the AEC is initialised, the
library functions can be called in a logical order to perform echo
cancellation on a frame by frame basis. Refer to the aec_1_thread and
aec_2_threads examples to see how the functions are called to perform
echo cancellation using one thread or 2 threads.
