==============================
Avona Host Control Application
==============================

This is the XMOS Avona host control application.


***** 
Setup
***** 

This example requires the xcore_sdk and Amazon Wakeword.

Set the environment variable XCORE_SDK_PATH to the root of the xcore_sdk and
set the environment variable WW_PATH to the root of the Amazon Wakeword.

.. tab:: Linux and MacOS

    .. code-block:: console

        $ export XCORE_SDK_PATH=/path/to/sdk
        $ export WW_PATH=/path/to/wakeword
        
.. tab:: Windows

    .. code-block:: x86 native tools command prompt
    
        > set XCORE_SDK_PATH=C:\path\to\sdk\
        > set WW_PATH=C:\path\to\wakeword\

LibUSB 0.1 and 1.0 are required for Linux and MacOS. Install these packages:
    libusb-dev
    libusb-1.0-0-dev
    
LibUSB-win32 is required for Windows.


************************
Building the Application
************************

To build this application, run the following commands in the host/control/ directory:

.. tab:: Linux and MacOS

    .. code-block:: console
    
        $ cmake -B build
        $ cd build
        $ make -j
        
.. tab:: Windows

    .. code-block:: x86 native tools command prompt
    
        > cmake -G "NMake Makefiles" -B build
        > cd build
        > nmake


*******************
Verify Installation
*******************

From the root folder of the example run:

.. tab:: Linux and MacOS

    .. code-block:: console

        $ avona_control.exe --help
        
.. tab:: Windows

    .. code-block:: x86 native tools command prompt
    
        > avona_control --help