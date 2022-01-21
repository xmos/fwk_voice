============================
Avona Voice Reference Design
============================

This is the XMOS Avona voice reference design

****************** 
Supported Hardware
****************** 

This example is supported on the XCORE-AI-EXPLORER board.

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
        
.. tab:: Windows XTC Tools CMD prompt

    .. code-block:: console
    
        $ set XCORE_SDK_PATH=C:\path\to\sdk\
        $ set WW_PATH=C:\path\to\wakeword\


*********************
Building the Firmware
*********************

Run the following commands to build the sw_avona firmware:

.. tab:: Linux and MacOS

    .. code-block:: console
    
        $ cmake -B build -DMULTITILE_BUILD=1 -DUSE_WW=amazon -DBOARD=XCORE-AI-EXPLORER -DXE_BASE_TILE=0 -DOUTPUT_DIR=bin
        $ cd build
        $ make -j
        
.. tab:: Windows XTC Tools CMD prompt

    .. code-block:: console
    
        $ cmake -G "NMake Makefiles" -B build -DMULTITILE_BUILD=1 -DUSE_WW=amazon -DBOARD=XCORE-AI-EXPLORER -DXE_BASE_TILE=0 -DOUTPUT_DIR=bin
        $ cd build
        $ nmake

After building the firmware, create the filesystem including the wakeword models and flash the device with the following commands:

Note, MacOS users will need to install `dosfstools`.

.. tab:: MacOS

    .. code-block:: console

        $ brew install dosfstools
        
.. tab:: Linux and MacOS

    .. code-block:: console

        $ cd filesystem_support
        $ ./flash_image.sh

.. tab:: Windows XTC Tools CMD prompt

    .. code-block:: console
    
        $ cd filesystem_support
        $ flash_image.bat


********************
Running the Firmware
********************

From the root folder of the example run:

    .. code-block:: console

        $ xrun --xscope bin/sw_avona.xe
