.. _vad_overview:

VAD Overview
============

The VAD (Voice Activity Detector) takes a stream of audio and estimates the probability of voice being present.

It takes one frame at a time, each of `VAD_FRAME_ADVANCE` samples, generates and keeps a history of audio features including MFCCs
(Mel-Frequency Cepstral Coefficients). These audio features are fed into a classifier consisting of a pre-trained
neural network which calculates the voice probability estimate.

VAD signals can be very helpful in voice processing pipelines. They provide an approximation of SNR (
Signal to Noise Ratio). Applications for VAD include intelligent power management, control of adaptive 
filters for reducing noise sources and improved performance of AGC (Automatic Gain Control) blocks that 
provide a more natural listening experience.

Before starting the VAD estimation the user must call vad_init() to initialise the VAD sate. There are no user configurable
parameters within the VAD and so no arguments are required and configuration structures need be tuned.

Once the VAD is initialised, the process function should be called on a frame by frame basis. If a break in VAD 
estimation is required then the initialisation function should be re-called before VAD is resumed. It will take
eight frames to be pushed (around 120ms) before VAD will be fully detecting voice activity.

