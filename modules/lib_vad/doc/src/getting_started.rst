.. _getting_started:

Getting Started
===============

Overview
--------

``lib_vad`` is a library which estimates the probability of voice being present in a given stream.
``lib_vad`` library functions uses``lib_xs3_math`` to perform DSP using low-level optimised operations. 

Repository Structure
--------------------

* ``modules/lib_vad`` - The actual ``lib_vad`` library directory within ``https://github.com/xmos/sw_avona/``.
  Within ``lib_vad``:

  * ``api/`` - Header file containing the public API for ``lib_vad``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
------------

``lib_vad`` is included as part of the ``sw_avona`` github repository
and all requirements for cloning and building ``sw_avona`` apply. ``lib_vad`` is compiled as a static library as part of
overall ``sw_avona`` build. It depends on ``lib_xs3_math``. 

API Structure
-------------

The API is presented as two simple functions. These are initialisation and processing. Initialisation is called once 
at startup and processing is called once per frame of samples. The performance requirement is relative low (around 5MIPS)
and as such is supplied as a single threaded implementation only.


Getting and Building
--------------------

This repo is obtained as part of the parent ``sw_avona`` repo clone. It is
compiled as a static library as part of ``sw_avona`` compilation process.

To include ``lib_vad`` in an application as a static library, the generated ``lib_vad.a`` can then be linked into the
application. Be sure to also add ``lib_vad/api`` as an include directory for the application.
