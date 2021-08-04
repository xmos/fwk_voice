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

Set the environment variable XCORE_SDK_PATH to the root of the xcore_sdk.

.. code-block:: console

    $ export XCORE_SDK_PATH=/path/to/sdk

Set the environment variable WW_PATH to the root of the Amazon Wakeword.

.. code-block:: console

    $ export WW_PATH=/path/to/wakeword


*********************
Building the Firmware
*********************

Run make:

.. code-block:: console

    $ make

After creating the binary, flash the wakeword model, then run the following commands:

.. code-block:: console

    $ cd filesystem_support
    $ ./flash_image.sh


********************
Running the Firmware
********************

From the root folder of the example run:

.. code-block:: console

    $ xrun --xscope bin/sw_xvf3652.xe

Or

.. code-block:: console

    $ make run
