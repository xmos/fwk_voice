
vnr
===

This example demonstrates how the VNR functions are called on a single thread to generate the Voice to Noise Ratio (VNR) estimates for an input audio stream.

In this example, a 32-bit, 1 channel wav file test_stream_1.wav is read and processed through the VNR frame by frame.
The neural network inference model used in the VNR is pre-trained to estimate the voice to noise ratio. It outputs a number between 0 and 1, 1 being the strongest voice with respect to noise and 0 being the lowest voice compared to noise ratio.

Building
********

After configuring the CMake project, the following commands can be used from the
`fwk_voice/examples/bare-metal/vnr <https://github.com/xmos/fwk_voice/tree/develop/examples/bare-metal/vnr>`_ directory to build and run this example application using the XCORE-AI-EXPLORER board as a target:
::
    
    cd ../../../build
    make fwk_voice_example_bare_metal_vnr_fileio
    cd ../examples/bare-metal/vnr
    python host_app.py test_stream_1.wav vnr_out.bin --run-with-xscope-fileio --show-plot

Alternatively, to not have the VNR output plot displayed on the screen, run,
::

    python host_app.py test_stream_1.wav vnr_out.bin --run-with-xscope-fileio

Output
******

The output from the VNR is written into the *vnr_out.bin* file. For every frame, VNR outputs its estimate in the form of a floating point value between 0 and 1. The floating point value is written as a 32bit mantissa, followed by a 32bit exponent in the vnr_out.bin file.
Additionally, these estimates are plotted, with the plot displayed on screen when run with the ``--show-plot`` argument. Irrespective of whether on not ``--show-plot`` is used as an option, the plotted figure is saved in the *vnr_example_plot_test_stream_1.png* file.
