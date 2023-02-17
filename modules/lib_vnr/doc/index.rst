Voice To Noise Ratio Estimator Library
=======================================

``lib_vnr`` is a library which estimates the ratio of speech signal in noise for an input audio stream.
``lib_vnr`` library functions uses ``lib_xcore_math`` to perform DSP using low-level optimised operations, and ``lib_tflite_micro`` and ``lib_nn`` to perform inference using an optimised TensorFlow Lite model.

.. toctree::
   :maxdepth: 1
   :caption: Contents

   src/getting_started
   src/overview
   src/reference/index

On GitHub
*********

``lib_vnr`` is present as part of ``fwk_voice``. Get the latest version of ``fwk_voice`` from
``https://github.com/xmos/fwk_voice``. The ``lib_vnr`` module can be found in the `modules/lib_vnr` directory in ``fwk_voice``.

