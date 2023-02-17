Interference Canceller Library
==============================

``lib_ic`` is a library which provides functions that together perform Interference Cancellation (IC)
on two channel input mic data by adapting to and modelling the room transfer characteristics. ``lib_ic`` library functions
make use of functionality provided in ``lib_aec`` for the core normalised LMS blocks which in turn uses
``lib_xcore_math`` to perform DSP low-level optimised operations. For more details refer to :ref:`ic_overview`.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   src/getting_started
   src/overview
   src/reference/index

On GitHub
*********

``lib_ic`` is present as part of ``fwk_voice``. Get the latest version of ``fwk_voice`` from
``https://github.com/xmos/fwk_voice``. The ``lib_ic`` module can be found in the `modules/lib_ic` directory in ``fwk_voice``.

API
***

To use the functions in this library in an application, include :ref:`ic_api_h` in the application source file
