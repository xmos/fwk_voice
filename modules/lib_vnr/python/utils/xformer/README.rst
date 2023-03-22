
Integrating a TensorFlow Lite model into the VNR module
=======================================================

This document describes the process for integrating a TensorFlow Lite model into the VNR module. Starting with an unoptimised model, follow the steps below to optimise it for XCORE by running it through the `xmos-ai-tools <https://pypi.org/project/xmos-ai-tools/>`_ xformer and integrate it into the VNR module.

1. Use the xformer to optimise the model for XCORE architecture.

2. Run the tflite_micro_compiler on the XCORE optimised model to generate the compiled .cpp and .h files that can be integrated in the VNR module. 

3. Update the TensorFlow Lite 8-bit quantization spec for the new model in `vnr_quant_spec_defines.h <https://github.com/xmos/fwk_voice/blob/develop/modules/lib_vnr/src/inference/model/vnr_quant_spec_defines.h>`_.

The `xform_model.py <https://github.com/xmos/fwk_voice/blob/develop/modules/lib_vnr/python/utils/xformer/xform_model.py>`_ script automates the above steps. It creates the files mentioned in steps 1-3 above and copies them to the VNR module directory. In addition to that, it also lists down the steps that the user is expected to do manually post running this script.
These steps include things making sure any old model files if present are deleted and the new files are added to git and all changes are committed. The script does provide a list of files that need removing and adding to git before committing to make this manual step easier.

Ensure you have installed Python 3 and the python requirements listed in `requirements.txt <https://github.com/xmos/fwk_voice/blob/develop/modules/lib_vnr/python/utils/xformer/requirements.txt>`_ in order to run the script. To use the script, run,

.. code-block:: console

    $ python xform_model.py <Unoptimised TensorFlow Lite model> --copy-files --module-path <path to model related files in lib_vnr module>

The above command will generate the relevant files and copy them into the VNR module.

For example, to run it for the existing model that we have, run,

.. code-block:: console

    $ python xform_model.py fwk_voice/modules/lib_vnr/python/model/model_output/trained_model.tflite --copy-files --module-path=fwk_voice/modules/lib_vnr/src/inference/model/


The process described above only generates an optimised model that would run on a single core.

Also worth mentioning is, since the feature extraction code is fixed and compiled as part of the VNR module, any new models replacing the existing one should have the same set of input features, input and output size and data types as the existing model.



