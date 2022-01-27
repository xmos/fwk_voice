Getting Started
===============

Overview
--------

``lib_agc`` is a library which performs Automatic Gain Control (AGC), with support for Loss Control.
For more details, refer to :ref:`agc_overview`.


Repository Structure
--------------------

* ``modules/lib_agc`` - The actual ``lib_agc`` library directory within ``https://github.com/xmos/sw_avona/``.
  Within ``lib_agc``

  * ``api/`` - Headers containing the public API for ``lib_agc``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
------------

``lib_agc`` is included as part of the ``sw_avona`` github repository and all requirements for cloning
and building ``sw_avona`` apply. ``lib_agc`` is compiled as a static library as part of the overall
``sw_avona`` build. It depends on `lib_xs3_math <https://github.com/xmos/lib_xs3_math/>`_.


Getting and Building
--------------------

This module is part of the parent ``sw_avona`` repo clone. It is compiled as a static library as part of
``sw_avona`` compilation process.

To include ``lib_agc`` in an application as a static library, the generated ``lib_agc.a`` can then be linked
into the application. Add ``lib_agc/api`` to the include directories when building the application.
