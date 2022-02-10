Getting Started
===============

Overview
--------

``lib_adec`` is a library which provides functions for measuring and correcting delay offsets between the reference
input and the loudspeaker output which is seen in the microphone input signal.
``lib_adec`` depends on ``lib_aec`` and ``lib_xs3_math`` libraries. For more details about the ADEC, refer to
:ref:`adec_overview`

Repository Structure
--------------------

* ``modules/lib_adec`` - The actual ``lib_adec`` library directory within ``https://github.com/xmos/sw_avona/``. Within ``lib_adec``

  * ``api/`` - Headers containing the public API for ``lib_adec``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.

Getting and Building
--------------------

``lib_adec`` is included as part of the ``sw_avona`` github repository
and all requirements for cloning and building ``sw_avona`` apply. ``lib_adec`` is compiled as a static library as part of
overall ``sw_avona`` build. To include ``lib_adec`` in an application as a static library, the generated ``lib_adec.a`` can then be linked into the
application. Be sure to also add ``lib_adec/api`` as an include directory for the application.





