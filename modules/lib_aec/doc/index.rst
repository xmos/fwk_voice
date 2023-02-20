Acoustic Echo Canceller Library
===============================

``lib_aec`` is a library which provides functions that can be put together to perform Acoustic Echo Cancellation (AEC)
on input mic data using the input reference data to model the room echo characteristics. ``lib_aec`` library functions
make use of functionality provided in ``lib_xcore_math`` to perform DSP operations. For more details refer to
:ref:`aec_overview`.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   src/getting_started
   src/overview
   src/reference/index


On GitHub
*********

``lib_aec`` is present as part of ``fwk_voice``. Get the latest version of ``fwk_voice`` from
``https://github.com/xmos/fwk_voice``. ``lib_aec`` is present within the `modules/lib_aec` directory in ``fwk_voice``

API
***

To use the functions in this library in an application, include :ref:`aec_api_h` in the application source file

