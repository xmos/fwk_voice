
Integrating a tflite model into the VNR module
==============================================

When integrating a new tflite model into the VNR module there are a number of steps involved.

- Use the transformer to convert from tflite to XCORE optimised tflite micro.
- Convert the tflite micro into data buffer that can be compiled into the VNR module.
- Get the tensor arena scratch memory requirement.
- Get the quantisation and dequantisation spec parameters.
- Call lib_tflite_micro functions to add all required operators to the resolver
- Update defines in xtflm_conf.h



