.. _getting_started:

Getting Started
===============

Overview
--------

``lib_ic`` is a library which provides functions that together perform Interference Cancellation (IC)
on two channel input mic data by modelling the room characteristics. ``lib_ic`` library functions
make use of functionality provided in ``lib_aec`` for core normalised LMS blocks which in turn uses
``lib_xs3_math`` to perform DSP operations. For more details refer toc :ref:`ic_overview`.

Repository Structure
--------------------

* `lib_ic/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_ic/>`_ - The actual ``lib_ic`` library directory.

  * `api/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_ic/api/>`_ - Headers containing the public API for ``lib_ic``.
  * `doc/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_ic/doc/>`_ - Library documentation source (for non-embedded documentation) and build directory.
  * `src/ <https://github.com/xmos/sw_avona/tree/develop/modules/lib_ic/src/>`_ - Library source code.


Requirements
------------

``lib_ic`` is included as part of the `sw_avona <https://github.com/xmos/sw_avona/tree/develop/>`_ github repository
and all requirements for cloning and building ``sw_avona`` apply. ``lib_ic`` is compiled as a static library as part of
overall ``sw_avona`` build. It depends on `lib_aec
<https://github.com/xmos/sw_avona/tree/develop/modules/lib_aec/>`_ and `lib_xs3_math
<https://github.com/xmos/sw_avona/tree/develop/modules/lib_xs3_math/>`_. 

API Structure
-------------

The API is presented as three simple functions. These are initialisation, filtering and adaption. Initialisation is called once 
and filtering and adaption is called once per frame of samples. The perormance requirement is relative low (less than 12MIPS)
and as such is supplied in a single threaded implementation only.


Getting and Building
--------------------

This repo is obtained as part of the parent `sw_avona <https://github.com/xmos/sw_avona/tree/develop/>`_ repo clone. It is
compiled as a static library as part of ``sw_avona`` compilation process.

To include ``lib_ic`` in an application as a static library, the generated ``lib_ic.a`` can then be linked into the
application. Be sure to also add ``lib_ic/api`` as an include directory for the application.
