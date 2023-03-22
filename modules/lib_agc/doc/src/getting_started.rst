Repository Structure
********************

* ``modules/lib_agc`` - The actual ``lib_agc`` library directory within ``https://github.com/xmos/fwk_voice/``.
  Within ``lib_agc``

  * ``api/`` - Headers containing the public API for ``lib_agc``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
************

``lib_agc`` is included as part of the ``fwk_voice`` github repository and all requirements for cloning
and building ``fwk_voice`` apply. ``lib_agc`` is compiled as a static library as part of the overall
``fwk_voice`` build. It depends on `lib_xcore_math <https://github.com/xmos/lib_xcore_math/>`_.


Getting and Building
********************

This module is part of the parent ``fwk_voice`` repo clone. It is compiled as a static library as part of
``fwk_voice`` compilation process.

To include ``lib_agc`` in an application as a static library, the generated ``libfwk_voice_module_lib_agc.a`` can then be linked
into the application. Add ``lib_agc/api`` to the include directories when building the application.
