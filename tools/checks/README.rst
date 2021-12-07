=====================
Avona Hardware Checks
=====================

This document describes the hardware checks for the Avona Voice Reference Design.  

*************
Prerequisites
*************

All checks require Linux or MacOS.  Most tests run on either the Explorer Board or the Avona reference design evaluation kit.

The following software applications are required.  If necessary, download and follow the installation instructions for each application.

- `SoX <http://sox.sourceforge.net/>`_ 
- `Python 3 <https://www.python.org/downloads/>`_ (We recommend and test with Python 3.8)

The following software applications are optional.

- `Docker <https://www.docker.com/>`_

Install the Python dependencies using the following commands:

Install pip if needed:

.. code-block:: console

    $ python -m pip install --upgrade pip

Then use pip to install the required modules.

.. code-block:: console

    $ pip install pytest

The checks require several firmware configurations.  To build and install all those configurations, run the following command in the root of the Avona repository:

.. code-block:: console

    $ ./tools/checks/build_check_configs.sh

******
Checks
******

Wakeword Detection
==================

The wakeword detection check verifies the ability of the Avona reference design to correctly detect wakewords.  It is currently only supported on the Explorer board and this test assumes you have set the WW_PATH environment variable to point to the the Amazon WakeWord library.

Run the following command to execute the check:

.. code-block:: console

    $ cd tools/checks
    $ ./check_wakeword_detection.sh -c 1 ../../dist/sw_avona_CHECK_USB_MICS.xe ${WW_PATH}/sample-wakeword/alexas.list | tee check_wakeword_detection.log

This generates the `check_wakeword_detection.log` log file.  

To verify the results, run:

.. code-block:: console

    $ pytest -s test_wakeword_detection.py

To compare this file to the output from the Amazon Wakeword `filesim` utility.  You can run the `filesim` utility with the following command:

.. code-block:: console

    $ cd ${WW_PATH}
    $ docker pull ubuntu
    $ docker run --rm -v ${WW_PATH}:/ww_path -w /ww_path ubuntu ./x86/amazon_ww_filesim -m models/common/WR_250k.en-US.alexa.bin sample-wakeword/alexas.list

.. note:: The detection start (and end) indices are not expected to match.  Only the detections are expected to match between the reference and Avona output.
