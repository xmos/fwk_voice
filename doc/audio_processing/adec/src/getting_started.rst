Repository Structure
********************

* ``modules/lib_adec`` - The actual ``lib_adec`` library directory within ``https://github.com/xmos/fwk_voice/``. Within ``lib_adec``

  * ``api/`` - Headers containing the public API for ``lib_adec``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.

Getting and Building
********************

``lib_adec`` is part of the ``fwk_voice`` repository and is provided as a CMake interface component,
with its source compiled as part of the consuming application. To use, link the ``fwk_voice::adec`` target in
the application's CMakeLists.txt and include ``adec.h`` in the application.





