
Integrating a TensorFlow Lite model into the VNR module
=======================================================

This document describes the process for integrating a TensorFlow Lite model into the VNR module. Starting with an unoptimised model, follow the steps below to optimise it for XCORE by running it through the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ xformer and integrate it into the VNR module.

1. Use the xformer to optimise the model for XCORE architecture.

2. Generate C source files `vnr_model_data.c <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/model/vnr_model_data.c>`_ and `vnr_model_data.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/model/vnr_model_data.h>`_ that contains the optimised model as a char array.

3. Update the tensor arena scratch memory requirement for the new model in `vnr_tensor_arena_size.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/model/vnr_tensor_arena_size.h>`_.

4. Update the TensorFlow Lite 8-bit quantization spec for the new model in `vnr_quant_spec_defines.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/model/vnr_quant_spec_defines.h>`_.

5. Call lib_tflite_micro functions to register all required operators with the lib_tflite_micro MicroOpResolver.

6. Update defines in `xtflm_conf.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/api/inference/xtflm_conf.h>`_

The `xform_model.py <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/python/utils/xformer/xform_model.py>`_ script automates the first 4 steps. It creates the files mentioned in steps 1-4 above and copies them to the VNR module directory. 
Ensure you have installed Python 3 and the python requirements listed in `requirements.txt <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/python/utils/xformer/requirements.txt>`_ in order to run the script. To use the script, run,

.. code-block:: console

    $ python xform_model.py <Unoptimised TensorFlow Lite model> --copy-files --module-path <path to model related files in lib_vnr module>

The above command will generate the relevant files and copy them into the VNR module.

For example, to run it for the existing model that we have, run,

.. code-block:: console

    $ python xform_model.py sw_avona/modules/lib_vnr/python/model/model_output/trained_model.tflite --copy-files --module-path=sw_avona/modules/lib_vnr/src/inference/model/


For step number 5. and 6.,

- Open the optimised model TensorFlow Lite file in netron.app and check the number and types of operators. Also check the number of input and output tensors. For example, the current optimised model `trained_model_xcore.tflite <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/model/trained_model_xcore.tflite>`_ uses 1 input and output tensor each and 4 operators; ``Conv2D``, ``Reshape``, ``Logistic`` and ``XC_conv2d_v2``.

- Add resolver->Addxxx functions in `vnr_inference.cc <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/vnr_inference.cc>`_, ``vnr_inference_init()`` to add all the operators to the TFLite operator resolver.

- In `xtflm_conf.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/api/inference/xtflm_conf.h>`_, update XTFLM_OPERATORS to indicate the correct number of operators.

- In `xtflm_conf.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/api/inference/xtflm_conf.h>`_, update NUM_OUTPUT_TENSORS and NUM_INPUT_TENSORS to indicate the correct numbers to input and output operators.


The process described above only generates optimised model to run on a single core.

Also worth mentioning is, since the feature extraction code is fixed and compiled as part of the VNR module, any new models replacing the existing one should have the same set of input features, input and output size and data types as the existing model.



