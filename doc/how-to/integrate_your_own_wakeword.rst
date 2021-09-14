##################################
Integrate Your Own Wakeword Engine
##################################

************
Introduction
************

This reference design is set up to allow for quickly changing between wakeword model runners.

Overview
========

Integration of a custom wakeword engine requires populating a template task and adding your code to the cmake build system.


**********************************
Adding Custom Wakeword Engine Code
**********************************

Procedure
=========

1. Starting at the root of the repository, move to the ww_model_runner directory.

.. code-block:: console

    $ cd src/ww_model_runner

2. Copy the template directory.

.. code-block:: console

    $ cp -r template my_custom_model_runner

3. Edit the files in my_custom_model_runner as needed for your specific model runner.

model_runner.c contains the FreeRTOS task that will run and contain your inference engine.  This template task receives appconfWW_FRAMES_PER_INFERENCE audio frames, which can be modified in app_conf.h.  The model_runner_manager task must consume frames faster than the audio pipeline pushes them or samples will be lost.

model_runner.cmake specifies the additional build requirements for your inference engine.  Here you can specify any additional build requirements, such as sources, includes, libraries to link, and preprocessor definitions.


**************************************
Building With a Custom Wakeword Engine
**************************************

To build with your custom wakeword engine, use the following command from the root of the repository:

.. code-block:: console

    $ make WW=my_custom_model_runner


***************
Troubleshooting
***************

This section contains some of the common issues and solutions.

Malloc Failed on tile x!
========================

This application uses dynamic memory allocation for all FreeRTOS tasks and RTOS primitives.

Likely issues are:
- User malloc of a buffer too large
- model_runner_manager requires a stack that is too large

Potential solutions:
- Increase the available heap for the kernel.  Modify configTOTAL_HEAP_SIZE in src/FreeRTOSConfig.h


lost output samples for ww
==========================

This message occurs when there was not available space in the stream buffer to the wakeword engine, when the audiopipeline had an output frame ready.

The likely issue is that the model_runner_manager task is taking too long to run.

Potential solutions:
- Increase the priority of the model_runner_manager task
