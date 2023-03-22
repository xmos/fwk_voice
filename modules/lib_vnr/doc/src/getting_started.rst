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

``lib_vnr`` is included as part of the ``fwk_voice`` github repository and all requirements for cloning and building ``fwk_voice`` apply. It depends on ``lib_xcore_math``, ``lib_tflite_micro`` and ``lib_nn``. 

API Structure
*************

The API is split into 2 parts; feature extraction and inference. The feature extraction API processes an input audio frame to extract features that are input to the inference stage. The inference API has functions for running inference using the VNR TensorFlow Lite model to predict the speech to noise ratio. Both feature extraction and inference APIs have initialisation functions that are called only once at device initialisation and processing functions that are called every frame.  
The performance requirement is relative low, around 5 MIPS for initialisation and 3 MIPS for processing, and as such is supplied as a single threaded implementation only.


Getting and Building
********************

The VNR estimator module is obtained as part of the parent ``fwk_voice`` repo clone. It is present in ``fwk_voice/modules/lib_vnr``

The feature extraction part of lib_vnr can be compiled as a static library. The application can link against ``libfwk_voice_module_lib_vnr_features.a`` and add ``lib_vnr/api/features`` and ``lib_vnr/api/common`` as include directories.
VNR inference engine compilation however, requires the runtime HW target to be specified, information about which is not available at library compile time. To include VNR inference engine in an application, it needs to compile the VNR inference related files from source. `lib_vnr module CMake file <https://github.com/xmos/fwk_voice/blob/develop/modules/lib_vnr/CMakeLists.txt>`_ demonstrates the VNR inference engine compiled as an INTERFACE library and if compiling using CMake, the application can simply `link` against the fwk_voice::vnr::inference library. For an example of compiling an application with VNR using CMake, refer to `VNR example CMake file <https://github.com/xmos/fwk_voice/blob/develop/examples/bare-metal/vnr/CMakeLists.txt>`_.

VNR Inference Model
*******************

The VNR estimator module uses a neural network model to predict the SNR of speech in noise for incoming data. The model used is a pre trained TensorFlow Lite model that has been optimised for the XCORE architecture using the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ xformer. The optimised model is compiled as part of the VNR Inference Engine. Changing the model at runtime is not supported. If changing to a different model, the application needs to generate the model related files, copy them to the appropriate directory within the VNR module and recompile. Part of this process is automated through a python script, as described below.

.. toctree::
   :maxdepth: 1

   xformer.rst

