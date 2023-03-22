Repository Structure
********************

* ``modules/lib_ic`` - The actual ``lib_ic`` library directory within ``https://github.com/xmos/fwk_voice/``.
  Within ``lib_ic``:

  * ``api/`` - Headers containing the public API for ``lib_ic``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
************

``lib_ic`` is included as part of the ``fwk_voice`` github repository
and all requirements for cloning and building ``fwk_voice`` apply. ``lib_ic`` is compiled as a static library as part of
overall ``fwk_voice`` build. It depends on ``lib_aec`` and ``lib_xcore_math``. 

API Structure
*************

The API is presented as three simple functions. These are initialisation, filtering and adaption. Initialisation is called once 
at startup and filtering and adaption is called once per frame of samples. The performance requirement is relative low (around 12MIPS)
and as such is supplied as a single threaded implementation only.


Getting and Building
********************

This repo is obtained as part of the parent ``fwk_voice`` repo clone. It is
compiled as a static library as part of ``fwk_voice`` compilation process.

To include ``lib_ic`` in an application as a static library, the generated ``libfwk_voice_module_lib_ic.a`` can then be linked into the
application. Be sure to also add ``lib_ic/api`` as an include directory for the application.
