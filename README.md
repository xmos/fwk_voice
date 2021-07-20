# Avona

This is the Next Generation Voice reference design

## Supported Hardware

This example is supported on the XCORE-AI-EXPLORER board.


## Setup

This example requires the xcore_sdk and Amazon Wakeword.

Set the environment variable XCORE_SDK_PATH to the root of the xcore_sdk.

    $ export XCORE_SDK_PATH=/path/to/sdk

Set the environment variable WW_PATH to the root of the Amazon Wakeword.

    $ export WW_PATH=/path/to/wakeword


## Building the Firmware

Run make:

    $ make

After creating the binary, flash the wakeword model, then run the following commands:

    $ cd filesystem_support
    $ ./flash_image.sh


## Running the Firmware

From the root folder of the example run:

    $ xrun --xscope bin/sw_xvf3652.xe

Or

    $ make run
