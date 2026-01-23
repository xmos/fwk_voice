
API Reference
-------------

AGC API Functions
^^^^^^^^^^^^^^^^^

.. doxygengroup:: agc_func

AGC Pre-Defined Profiles and Parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Three pre-defined profiles are provided in `agc_profiles.h` to configure the AGC for different applications:

.. doxygengroup:: agc_profiles

These profiles can be used to configure the AGC instance by passing them to the
:c:func:`agc_init` function.

.. _agc_profiles:

AGC Parameters
^^^^^^^^^^^^^^

The key AGC parameters are highlighted below:

.. Descriptions extracted from agc.h struct documentation to avoid duplication

* :c:member:`agc_config_t.adapt` - Boolean to enable AGC adaption; if enabled, the gain to apply will adapt based on the peak of the input frame and the upper/lower threshold parameters.
* :c:member:`agc_config_t.vnr_threshold` - VNR threshold for voice activity detection. A higher value will only adapt the AGC on clean speech. A lower value will adapt the AGC on noisy speech, but may also adapt to more non-speech signals.
* :c:member:`agc_config_t.gain` - The current gain to be applied, not including loss control. When `adapt` is false, this gain will be applied to every frame. When `adapt` is true, the initial value of this gain will be applied to the first frame and then it will be adapted on subsequent frames.
* :c:member:`agc_config_t.max_gain` - The maximum gain allowed when adaption is enabled. This can be used to prevent the AGC amplifying very quiet signals.
* :c:member:`agc_config_t.upper_threshold` - The target maximum peak level of the AGC output. If the AGC output goes above this level, the gain is reduced.
* :c:member:`agc_config_t.lower_threshold` - The target minimum peak level of the AGC output. If the AGC output goes below this level, the gain is increased.
* :c:member:`agc_config_t.soft_clipping` - Boolean to enable soft-clipping of the output frame.
* :c:member:`agc_config_t.lc_enabled` - Boolean to enable loss control. The loss control applies additional attenuation when there is no near end speech. This must be disabled if the application doesn't have an AEC or VNR.
* :c:member:`agc_config_t.lc_near_delta` - Delta multiplier used when only near-end activity is detected. How many times louder the near-end signal must be than the background noise when there is no far-end playback. If the near end speech is not heard during silence, reduce this value. If too much non-speech background noise is heard, increase this value.
* :c:member:`agc_config_t.lc_near_delta_far_active` - Delta multiplier used when both near-end and far-end activity is detected. How many times louder the near end signal must be above the residual far-end speech (after the AEC) to be detected during double talk. If the near end speech is not heard during double talk, reduce this value. If there is too much breakthrough of residual far-end echo when there is no near-end speech present, increase this value.
* :c:member:`agc_config_t.lc_gain_double_talk` - Loss control gain to apply when double-talk is detected. Reducing this value will reduce the level of the near-end speech during double-talk, but may help to reduce the level of residual far-end echo that is heard.



Other AGC parameters are described in the `agc_profiles.h` header file,
and are described in detail in :c:struct:`agc_config_t`.

AGC API Structure Definitions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. doxygengroup:: agc_defs
    :members:

