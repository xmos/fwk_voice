==============================
Avona Host Control Application
==============================

This is the XMOS Avona host control application.


***** 
Setup
***** 

This example requires the xcore_sdk and Amazon Wakeword.

Set the environment variable XCORE_SDK_PATH to the root of the xcore_sdk and

.. tab:: Linux and MacOS

    .. code-block:: console

        $ export XCORE_SDK_PATH=/path/to/sdk
        
.. tab:: Windows

    .. code-block:: x86 native tools command prompt
    
        > set XCORE_SDK_PATH=C:\path\to\sdk\

LibUSB 1.0 is required for Linux. Install these packages:

- libusb-dev
- libusb-1.0-0-dev  

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

        $ avona_control --help
        
.. tab:: Windows

    .. code-block:: x86 native tools command prompt
    
        > avona_control.exe --help
