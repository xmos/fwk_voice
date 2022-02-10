.. _adec_overview:

ADEC Overview
~~~~~~~~~~~~~~

The ADEC module provides functions to estimate and automatically correct for possible delay offsets between the reference and the loudspeakers.

Acoustic echo cancellation is an adaptive filtering process which compares the reference audio to that received from the
microphones.  It models the reverberant time of a room, i.e. the time it takes for acoustic reflections to decay to
insignificance. The time window modelled by the AEC is finite, and to maximise its performance it is important to ensure that the
reference audio is presented to the AEC time aligned to the audio being reproduced by the loudspeakers.  The reference
audio path delay and the audio reproduction path may delay be significantly different, requiring additional delay to be
inserted into one of the two paths, to correct this delay difference.

ADEC module provides functionality for measuring the current mic-ref delay and using the measured delay along with
AEC perfomance related metadata collected from the echo canceller to monitor the AEC and make decisions about issuing requests for reconfiguring the AEC and
correcting bulk delay offsets.


