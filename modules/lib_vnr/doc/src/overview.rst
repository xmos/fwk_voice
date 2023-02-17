.. _vnr_overview:

VNR Overview
************

The VNR (Voice to Noise Ratio) estimator predicts the signal to noise ratio of a speech signal in noise, using a pre-trained neural network. The VNR neural network model outputs a value between 0 and 1, with 1 indicating the strongest speech, and 0, the weakest speech compared to noise in a frame of audio data.

The VNR module processes `VNR_FRAME_ADVANCE` new audio pcm samples every frame. The time domain input is transformed to frequency domain using a 512 point DFT. A MEL filterbank is then applied to compress the DFT output spectrum into fewer data points. The MEL filter outputs of `VNR_PATCH_WIDTH` most recent frames are normalised and fed as input features to the VNR prediction model which runs an inference over the features to output the VNR estimate value.

VNR estimations can be very helpful in voice processing pipelines. Applications for VNR include intelligent power management, control of adaptive 
filters for reducing noise sources and improved performance of AGC (Automatic Gain Control) blocks that provide a more natural listening experience.

The VNR API is split into 2 parts; feature extraction and inference. This is done to allow multiple sets of features to use the same inference engine.
The VNR feature extraction is further split into 2 parts; a function to form the input frame that the feature extraction can run on, and a function to do the actual feature extraction. The function for forming the input frame starts from `VNR_FRAME_ADVANCE` new pcm samples and creates the DFT output that is used as input to the MEL filterbank. This has been separated from the rest of the feature extraction to support cases where the VNR might be using the DFT output computed in another module for extracting features.

The pre-trained, optimised for XCORE TensorFlow Lite model, that is used for VNR inference has been compiled as part of the VNR inference static library. There's no support for providing a new model to the inference engine at run time.

Before starting the feature extraction, the user must call ``vnr_input_state_init()`` and ``vnr_feature_state_init()`` to initialise the form input frame and feature extraction state. Before starting inference, the user must call ``vnr_inference_init()`` to initialise the inference engine.

There are no user configurable parameters within the VNR and so no arguments are required and no configuration structures need be tuned.

Once the VNR is initialised, the ``vnr_form_input_frame()``, ``vnr_extract_features()`` and ``vnr_inference()`` functions should be called on a frame by frame basis.
