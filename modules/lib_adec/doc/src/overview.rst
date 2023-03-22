.. _adec_overview:

ADEC Overview
*************

The ADEC module provides functions to estimate and automatically correct for delay offsets between the reference and the
loudspeakers.

Acoustic echo cancellation is an adaptive filtering process which compares the reference audio to that received from the
microphones.  It models the reverberation time of a room, i.e. the time it takes for acoustic reflections to decay to
insignificance. The time window modelled by the AEC is finite, and to maximise its performance it is important to ensure
that the reference audio is presented to the AEC time aligned to the audio being reproduced by the loudspeakers. The
reference audio path delay and the audio reproduction path delay may be significantly different, requiring additional
delay to be inserted into one of the two paths, to correct this delay difference.

The ADEC module provides functionality for 

* Measuring the current delay
* Using the measured delay along with AEC performance related metadata collected from the echo canceller to monitor AEC and make decisions about reconfiguring the AEC and correcting bulk delay offsets.

The metadata collected from AEC contains statistics such as the ERLE, the peak power seen in the adaptive filter and the
peak power to average power ratio of the adaptive filter.

The ADEC algorithm works in 2 modes - normal mode and delay estimation mode.
In its normal mode ADEC monitors the AEC performance and requests small delay corrections. Using the statistics from the AEC, the ADEC estimates a metric called the
AEC goodness which is an estimate of how well the echo canceller is performing. Based on the estimated AEC goodness and the current measured delay, the ADEC can
request for a delay correction to be applied at the input of the echo canceller.

If the AEC is seen as consistently bad, the ADEC transitions to a delay estimation mode and requests for

* A special delay to be applied at AEC input that will enable measuring the actual delay in both delay scenarios; microphone input arriving at the AEC earlier in time than the reference input as well as microphone input arriving late in time wrt reference input.
* A restart of AEC in a new configuration that has more adaptive filter phases, in order of have a longer filter tail length that is suitable for delay estimation.

Once the ADEC has a measure of the new delay, it requests a delay correction and a reconfiguration of the AEC back to its normal
mode and goes back to its normal mode of monitoring AEC performance and correcting for small delay offsets.

Before processing any frames, the application must configure and initialise the ADEC instance by calling adec_init(). Then for each frame, adec_estimate_delay() will estimate the current delay and adec_process_frame() will use the current frame's AEC statistics and the estimated delay to monitor the AEC and request possible AEC and delay configuration changes.


