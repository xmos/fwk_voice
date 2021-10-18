====================
Avona Hardware Tests
====================

This document describes the hardware tests for the Avona Voice Reference Design.  

*************
Prerequisites
*************

All tests require Linux or MacOS.  Most tests run on either the Explorer Board or the Avona reference design evaluation kit.

The following software applications are required.  If necessary, download and follow the installation instructions for each application.

- `SoX <http://sox.sourceforge.net/>`_ 
- `Docker <https://www.docker.com/>`_ (MacOS and Windows only)

The tests require several firmware configurations.  To build and install all those configurations, run the following command in the root of the Avona repository:

.. code-block:: console

    $ ./test/build_test_configs.sh

*****
Tests
*****

Wakeword Detection
==================

The wakeword detection test verifies the ability of the Avona reference design to correctly detect wakewords.  It is currently only supported on the Explorer board and this test assumes you have set the WW_PATH environment variable to point to the the Amazon WakeWord library.

Run the following command to execute the test:

.. code-block:: console

    $ cd test
    $ ./test_wakeword_detection.sh -c 1 ../dist/sw_avona_TEST_USB_MICS.xe ${WW_PATH}/sample-wakeword/alexas.list | tee test_wakeword_detection.log

This generates the `test_wakeword_detection.log` log file.  Compare this file to the output from the Amazon Wakeword `filesim` utility.  You can run the `filesim` utility with the following command:

On Linux run:

.. code-block:: console

    $ cd ${WW_PATH}
    $ ./x86/amazon_ww_filesim -m models/common/WR_250k.en-US.alexa.bin sample-wakeword/alexas.list

On MacOS or Windows run:

.. code-block:: console

    $ cd ${WW_PATH}
    $ docker pull ubuntu
    $ docker run --rm -v ${WW_PATH}:/ww_path -w /ww_path ubuntu ./x86/amazon_ww_filesim -m models/common/WR_250k.en-US.alexa.bin sample-wakeword/alexas.list

.. note:: The detection start (and end) indices are not expected to match.  Only the detections are expected to match between the reference and Avona output.
