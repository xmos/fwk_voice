.. _adec_overview:

ADEC Overview
~~~~~~~~~~~~~~

The ADEC module provides functions to estimate and automatically correct for delay offsets between the reference and the
loudspeakers.

Acoustic echo cancellation is an adaptive filtering process which compares the reference audio to that received from the
microphones.  It models the reverberation time of a room, i.e. the time it takes for acoustic reflections to decay to
insignificance. The time window modelled by the AEC is finite, and to maximise its performance it is important to ensure
that the reference audio is presented to the AEC time aligned to the audio being reproduced by the loudspeakers.  The
reference audio path delay and the audio reproduction path delay may be significantly different, requiring additional
delay to be inserted into one of the two paths, to correct this delay difference.

ADEC module provides functionality for 

* Measuring the current mic-ref delay
* Using the measured delay along with AEC perfomance related metadata collected from the echo canceller to monitor AEC and make decisions about reconfiguring the AEC and correcting bulk delay offsets.

The metadata collected from AEC contains statistics such as the ERLE, the peak power seen in the adaptive filter and the
peak power to average power ratio of the adaptive filter. Using these statistics ADEC estimates a metric called the AEC
goodness which is an estimate of how well the echo canceller is performing. Based on the estimated AEC goodness and the
current measured delay, ADEC can request for a delay correction to be applied at the input of the echo canceller.  If
the AEC is seen as consistently bad, the ADEC can also request a restart of the AEC with a different configuration; one
that is suitable for measuring delay in both directions so that ADEC can monitor and estimate a new delay.


