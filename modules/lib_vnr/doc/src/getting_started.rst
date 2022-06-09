.. _getting_started:

Getting Started
===============

Overview
--------

``lib_vnr`` is a library which estimates the ratio of speech signal in noise for an input audio stream.
``lib_vnr`` library functions uses ``lib_xs3_math`` to perform DSP using low-level optimised operations, and ``lib_tflite_micro`` and ``lib_nn`` to perform inference using an optimised TensorFlow Lite model.

Repository Structure
--------------------

* ``modules/lib_vnr`` - The ``lib_vnr`` library directory within ``https://github.com/xmos/sw_avona/``.
  Within ``lib_vnr``:

  * ``api/`` - Header files containing the public API for ``lib_vnr``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
------------

``lib_vnr`` is included as part of the ``sw_avona`` github repository and all requirements for cloning and building ``sw_avona`` apply. ``lib_vnr`` is compiled as a static library as part of overall ``sw_avona`` build. It depends on ``lib_xs3_math``, ``lib_tflite_micro`` and ``lib_nn``. 

API Structure
-------------

The API is split into 2 parts; feature extraction and inference. The feature extraction API processes an input audio frame to extract features that are input to the inference stage. The inference API has functions for running inference using the VNR TensorFlow Lite model to predict the speech to noise ratio. Both feature extraction and inference APIs have initialisation functions that are called only once at device initialisation and processing functions that are called every frame.  
The performance requirement is relative low, around 5 MIPS for initialisation and 3 MIPS for processing, and as such is supplied as a single threaded implementation only.


Getting and Building
--------------------

This repo is obtained as part of the parent ``sw_avona`` repo clone. It is compiled as a static library as part of ``sw_avona`` compilation process.

To include ``lib_vnr`` in an application as a static library, ``libavona_module_lib_vnr_features.a`` and ``libavona_module_lib_vnr_inference.a`` can be linked into the application. Be sure to also add ``lib_vnr/api/features`` and ``lib_vnr/api/inference`` as an include directory for the application.
