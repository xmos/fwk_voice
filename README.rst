================================
XMOS Voice Framework DSP Library
================================

This is the XMOS Voice Framework DSP library which contains high-performance audio processing algorithms, optimized for xcore.ai.

******************
Supported Hardware
******************

This library is designed to be used with all xcore.ai hardware.

*****
Setup
*****

Using this library requires CMake, version 3.21 or greater.

In your top level CMakeLists.txt, include this repository in your preferred manner, and add_subdirectory(fwk_voice).  Then invoke CMake with the desired toolchain.

The fwk_voice CMake will define various targets that can be linked to your application.  In your CMake build folder, you can use the following command to display available targets:

.. code-block:: console

    $ make help
