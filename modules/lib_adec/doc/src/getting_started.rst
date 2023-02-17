Getting Started
===============

Overview
--------

``lib_adec`` is a library which provides functions for measuring and correcting delay offsets between the reference
and loudspeaker signals.
``lib_adec`` depends on ``lib_aec`` and ``lib_xcore_math`` libraries. For more details about the ADEC, refer to
:ref:`adec_overview`

Repository Structure
--------------------

* ``modules/lib_adec`` - The actual ``lib_adec`` library directory within ``https://github.com/xmos/fwk_voice/``. Within ``lib_adec``

  * ``api/`` - Headers containing the public API for ``lib_adec``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.

Getting and Building
--------------------

``lib_adec`` is included as part of the ``fwk_voice`` github repository
and all requirements for cloning and building ``fwk_voice`` apply. ``lib_adec`` is compiled as a static library as part of
overall ``fwk_voice`` build. To include ``lib_adec`` in an application as a static library, the generated ``libfwk_voice_module_lib_adec.a`` can then be linked into the application. Be sure to also add ``lib_adec/api`` as an include directory for the application.





