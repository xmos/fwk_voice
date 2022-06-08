
Integrating a tflite model into the VNR module
==============================================

Follow the steps below when integrating a new tflite model into the VNR module -

1. Use the transformer to convert from tflite to XCORE optimised tflite micro.
2. Convert the tflite micro into data buffer that can be compiled into the VNR module.
3. Get the tensor arena scratch memory requirement.
4. Get the quantisation and dequantisation spec parameters.
5. Call lib_tflite_micro functions to add all required operators to the resolver
6. Update defines in xtflm_conf.h

The convert_model.py script automates the first 4 steps. To use the script, run

python convert_model.py <tflite model> --copy-files --module-path <path to lib_vnr module>

The above command will generate the relevant files and copy them into the VNR module.

For example, to run it for the existing model that we have, run,

python convert_model.py ../model/model_output/model_qaware.tflite --copy-files --module-path=../../lib_vnr


For step number 5. and 6.,

- Open the optimised model tflite file in netron.app and check the number and type of operators. Also check the number of input and output tensors.

- Add resolver->Addxxx functions in vnr_inference.cc, vnr_inference_init() to add all the operators.

- Update XTFLM_OPERATORS to indicate the correct number of operators.

- Update NUM_OUTPUT_TENSORS and NUM_INPUT_TENSORS to indicate the correct numbers to input and output operators.





