.. _getting_started:

Repository Structure
********************

* ``modules/lib_vnr`` - The ``lib_vnr`` library directory within ``https://github.com/xmos/fwk_voice/``.
  Within ``lib_vnr``:

  * ``api/`` - Header files containing the public API for ``lib_vnr``.
  * ``doc/`` - Library documentation source (for non-embedded documentation) and build directory.
  * ``src/`` - Library source code.


Requirements
************

``lib_vnr`` is included as part of the ``fwk_voice`` github repository and all requirements for cloning and building ``fwk_voice`` apply. It depends on ``lib_xcore_math``
and an `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ python package.

API Structure
*************

The API is split into 2 parts; feature extraction and inference. The feature extraction API processes an input audio frame to extract features that are input to the inference stage.
The inference API has functions for running inference using the VNR TensorFlow Lite model to predict the speech to noise ratio. 
Both feature extraction and inference APIs have initialisation functions that are called only once at device initialisation and processing functions that are called every frame.  
The performance requirement is relative low, around 5 MIPS for initialisation and 3 MIPS for processing, and as such is supplied as a single threaded implementation only.


Getting and Building
********************

The VNR estimator module is obtained as part of the parent ``fwk_voice`` repo clone. It is present in ``fwk_voice/modules/lib_vnr``

Both feature extraction and the inference parts of ``lib_vnr`` can be compiled as static libraries. The application can link against ``libfwk_voice_module_lib_vnr_features.a`` 
and/or ``libfwk_voice_module_lib_vnr_inference.a`` and add ``lib_vnr/api/features`` and/or ``lib_vnr/api/inference`` and ``lib_vnr/api/common`` as include directories.

VNR Inference Model
*******************

The VNR estimator module uses a neural network model to predict the SNR of speech in noise for incoming data. The model used is a pre trained TensorFlow Lite model 
that has been optimised for the XCORE architecture using the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ xformer. 
The optimised model is compiled as part of the VNR Inference Engine. Changing the model at runtime is not supported. 
If changing to a different model, the application needs to generate the model related files and recompile. 
This process is automated through the build system, as described below.

Integrating a TensorFlow Lite model into the VNR module
=======================================================

To integrate the new TensorFlow Lite model into the VNR module:

#. Put an unoptimised model into ``fwk_voice/modules/lib_vnr/python/model/model_output/trained_model.tflite``

#. Rerun the build tool of our choice (``make`` or ``ninja``, for example)

This will use `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ to optimise ``.tflite`` model for xcore and generate ``.cpp`` and ``.h`` files
into ``fwk_voice/modules/lib_vnr/src/inference/model/``. Those generated files will be picked by the build system and compiled into the VNR module.

The process described above only generates an optimised model that would run on a single core.

Also worth mentioning is, since the feature extraction code is fixed and compiled as part of the VNR module,
any new models replacing the existing one should have the same set of input features,
input and output size and data types as the existing model.
