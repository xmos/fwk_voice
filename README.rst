=============================
Avona Audio DSP Library
=============================

This is the XMOS Avona DSP library which contains high-performance audio processing algorithms, optimized for xcore.ai.

******************
Supported Hardware
******************

This library is designed to be used with all xcore.ai hardware.

*****
Setup
*****

Using this library requires CMake, version 3.21 or greater.

In your top level CMakeLists.txt, include this repository in your preferred manor, and add_subdirectory(sw_avona).  Then invoke CMake with the desired toolchain.

The sw_avona CMake will define various targets that can be linked to your application.  In your CMake build folder, you can use the following command to display available targets:

.. code-block:: console

    $ make help
