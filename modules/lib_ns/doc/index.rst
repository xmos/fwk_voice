Noise Suppression Library
=========================

``lib_ns`` is a library which performs Noise Suppression (NS), by estimating the noise and 
subtracting it from frame. ``lib_ns`` library functions make use of functionality 
provided in ``lib_xcore_math`` to perform DSP operations. For more details, refer to :ref:`ns_overview`.

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   src/getting_started
   src/overview
   src/reference/index

On GitHub
*********

``lib_ns`` is present as part of ``fwk_voice``. Get the latest version of ``fwk_voice`` from
``https://github.com/xmos/fwk_voice``. ``lib_ns`` is present within the `modules/lib_ns` directory in ``fwk_voice``.

API
***

To use the functions in this library in an application, include :ref:`ns_api_h` in the application source file.

