Automatic Delay Estimation and Correction Library
=================================================

``lib_adec`` is a library which provides functions for measuring and correcting delay offsets between the reference
and loudspeaker signals.
``lib_adec`` depends on ``lib_aec`` and ``lib_xcore_math`` libraries. For more details about the ADEC, refer to
:ref:`adec_overview`

.. toctree::
   :maxdepth: 1
   :caption: Contents:

   src/getting_started
   src/overview
   src/reference/index

On GitHub
---------

``lib_adec`` is present as part of ``fwk_voice``. Get the latest version of ``fwk_voice`` from
``https://github.com/xmos/fwk_voice``. ``lib_adec`` is present within the `modules/lib_adec` directory in ``fwk_voice``

API
---

To use the functions in this library in an application, include :ref:`adec_api_h` in the application source file

