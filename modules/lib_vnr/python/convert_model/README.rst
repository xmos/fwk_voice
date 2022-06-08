
Integrating a tflite model into the VNR module
==============================================

Follow the steps below when integrating a new tflite model into the VNR module -

1. Use the transformer to convert from tflite to XCORE optimised tflite micro.

2. Generate C source file that contains the TensorFlow Lite model as a char array.

3. Get the tensor arena scratch memory requirement.

4. Get the quantisation and dequantisation spec parameters.

5. Call lib_tflite_micro functions to add all required operators to the resolver.

6. Update defines in xtflm_conf.h

The convert_model.py script automates the first 4 steps. Ensure you have installed Python 3 and the python requirements listed in requirements.txt in order to run the script. To use the script, run,

.. code-block:: console

    $ python convert_model.py <tflite model> --copy-files --module-path <path to lib_vnr module>

The above command will generate the relevant files and copy them into the VNR module.

For example, to run it for the existing model that we have, run,

.. code-block:: console

    $ python convert_model.py <sw_avona repo path>/modules/lib_vnr/python/model/model_output/model_qaware.tflite --copy-files --module-path=<sw_avona repo path>/modules/lib_vnr


For step number 5. and 6.,

- Open the optimised model tflite file in netron.app and check the number and types of operators. Also check the number of input and output tensors. For example, the current optimised model /modules/lib_vnr/python/model/model_output/model_qaware_xcore.tflite uses 4 operators; Conv2D, Reshape, Logistic and XC_conv2d_v2.

- Add resolver->Addxxx functions in `vnr_inference.cc <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/vnr_inference.cc>`_, vnr_inference_init() to add all the operators to the TFLite operator resolver.

- In `xtflm_conf.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/xtflm_conf.h>`_, update XTFLM_OPERATORS to indicate the correct number of operators.

- In `xtflm_conf.h <https://github.com/xmos/sw_avona/blob/develop/modules/lib_vnr/src/inference/xtflm_conf.h>`_, update NUM_OUTPUT_TENSORS and NUM_INPUT_TENSORS to indicate the correct numbers to input and output operators.





