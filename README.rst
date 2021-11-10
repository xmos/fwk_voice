============================
Avona Voice Reference Design
============================

This is the XMOS Avona voice reference design

****************** 
Supported Hardware
****************** 

This software is supported on the XCORE-AI-EXPLORER and XVF3610_Q60A boards.

***** 
Setup
***** 

This software requires the xcore_sdk and Amazon Wakeword.

Set the environment variable XCORE_SDK_PATH to the root of the xcore_sdk.

.. code-block:: console

    $ export XCORE_SDK_PATH=/path/to/sdk

Set the environment variable WW_PATH to the root of the Amazon Wakeword deliverable.

.. code-block:: console

    $ export WW_PATH=/path/to/wakeword


*********************
Building the Firmware
*********************

Change to the avona application folder:

.. code-block:: console

    $ cd applications/avona

Run make to build the firmware:

.. code-block:: console

    $ make

After building the firmware, create the filesystem including the wakeword models and flash the device with the following commands:

Note, MacOS users will need to install `dosfstools`.

.. code-block:: console

    $ brew install dosfstools

.. code-block:: console

    $ cd filesystem_support
    $ ./flash_image.sh


********************
Running the Firmware
********************

From the root folder of the application run:

.. code-block:: console

    $ xrun --xscope bin/sw_avona.xe

Or

.. code-block:: console

    $ make run
